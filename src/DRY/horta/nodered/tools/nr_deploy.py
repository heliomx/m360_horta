#!/usr/bin/env python3
"""
Deploy do flows.json no Node-RED (nr.viridiotech.com.br) via API admin.

Autenticação: usuário/senha lidos de um arquivo FORA do repositório
(%USERPROFILE%\\.m360\\nodered.env, ou NODE_RED_CREDENTIALS=<caminho>).
O script faz login (POST /auth/token), usa o token de curta duração e o
revoga ao terminar. Nunca grave token nem senha no repositório.

Subcomandos:
  login                         testa as credenciais
  backup                        salva GET /flows em ~/.m360/backups/nodered/
  diff                          compara, aba a aba, flows.json local x servidor
  pull                          grava o estado do SERVIDOR no flows.json local
  deploy --tab ID [ID ...]      PUT /flow/{id} só das abas indicadas (backup antes)
  deploy --all --yes            PUT /flows com o arquivo inteiro (substitui TUDO)

Regras (ver DEPLOY.md):
  * Sempre `backup` + `diff` antes de `deploy`.
  * Se o diff mostrar nós "só no servidor", o servidor está à frente: fazer
    `pull` e reaplicar a mudança local, nunca gravar a aba por cima.
  * A aba Irrigação só sobe depois de Nó 99 e gateway gravados com o failsafe.
  * `deploy --all` substitui config nodes (broker MQTT, dashboard, Telegram).
"""
import argparse
import copy
import datetime as dt
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
FLOWS_LOCAL = os.path.normpath(os.path.join(HERE, "..", "flows.json"))
CRED_FILE = os.environ.get(
    "NODE_RED_CREDENTIALS",
    os.path.join(os.path.expanduser("~"), ".m360", "nodered.env"),
)
BACKUP_DIR = os.path.join(os.path.expanduser("~"), ".m360", "backups", "nodered")


# --------------------------------------------------------------------- infra
def load_credentials():
    if not os.path.isfile(CRED_FILE):
        sys.exit(f"[erro] arquivo de credenciais não encontrado: {CRED_FILE}")
    env = {}
    with open(CRED_FILE, encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            env[k.strip()] = v.strip()
    missing = [k for k in ("NODE_RED_URL", "NODE_RED_USERNAME", "NODE_RED_PASSWORD") if not env.get(k)]
    if missing:
        sys.exit(f"[erro] faltam em {CRED_FILE}: {', '.join(missing)}")
    env["NODE_RED_URL"] = env["NODE_RED_URL"].rstrip("/")
    return env


class NodeRed:
    def __init__(self, env):
        self.base = env["NODE_RED_URL"]
        self.user = env["NODE_RED_USERNAME"]
        self._pwd = env["NODE_RED_PASSWORD"]
        self.token = None

    # -- HTTP ------------------------------------------------------------
    def _req(self, method, path, body=None, headers=None, form=False):
        url = self.base + path
        data = None
        hdrs = {"Accept": "application/json"}
        if self.token:
            hdrs["Authorization"] = "Bearer " + self.token
        if body is not None:
            if form:
                data = urllib.parse.urlencode(body).encode()
                hdrs["Content-Type"] = "application/x-www-form-urlencoded"
            else:
                data = json.dumps(body, ensure_ascii=False).encode("utf-8")
                hdrs["Content-Type"] = "application/json"
        hdrs.update(headers or {})
        req = urllib.request.Request(url, data=data, method=method, headers=hdrs)
        try:
            with urllib.request.urlopen(req, timeout=60) as resp:
                raw = resp.read()
                return resp.status, (json.loads(raw) if raw else None)
        except urllib.error.HTTPError as e:
            raw = e.read().decode("utf-8", "replace")
            raise SystemExit(f"[erro] {method} {path} -> HTTP {e.code}: {raw[:500]}")

    def login(self):
        _, data = self._req("POST", "/auth/token", form=True, body={
            "client_id": "node-red-admin", "grant_type": "password",
            "scope": "*", "username": self.user, "password": self._pwd,
        })
        self.token = data.get("access_token")
        if not self.token:
            sys.exit("[erro] login sem access_token")
        return self

    def logout(self):
        if self.token:
            try:
                self._req("POST", "/auth/revoke", body={"token": self.token})
            except SystemExit:
                pass
            self.token = None

    # -- API -------------------------------------------------------------
    def get_flows(self):
        _, data = self._req("GET", "/flows", headers={"Node-RED-API-Version": "v2"})
        return data  # {"rev": ..., "flows": [...]}

    def get_flow(self, fid):
        _, data = self._req("GET", f"/flow/{fid}")
        return data

    def put_flow(self, fid, payload):
        _, data = self._req("PUT", f"/flow/{fid}", body=payload)
        return data

    def put_flows(self, flows, rev):
        _, data = self._req("PUT", "/flows", body={"rev": rev, "flows": flows},
                            headers={"Node-RED-API-Version": "v2",
                                     "Node-RED-Deployment-Type": "full"})
        return data


# ------------------------------------------------------------------ helpers
def load_local():
    with open(FLOWS_LOCAL, encoding="utf-8") as fh:
        return json.load(fh)


def save_backup(obj, label):
    os.makedirs(BACKUP_DIR, exist_ok=True)
    stamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    path = os.path.join(BACKUP_DIR, f"{stamp}_{label}.json")
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(obj, fh, indent=2, ensure_ascii=False)
    return path


def normalize(node):
    """Remove diferenças que o runtime introduz e que não são divergência real."""
    n = copy.deepcopy(node)
    n.pop("x", None)
    n.pop("y", None)
    if n.get("type") == "link out" and n.get("mode") == "link":
        n.pop("wires", None)  # o runtime descarta wires: [] de link out
    if "wires" in n and n["wires"] == []:
        n.pop("wires")
    return n


def by_tab(flows):
    tabs = {n["id"]: n for n in flows if n["type"] == "tab"}
    members = {tid: {} for tid in tabs}
    globals_ = {}
    for n in flows:
        if n["type"] == "tab":
            continue
        z = n.get("z")
        if z in members:
            members[z][n["id"]] = n
        elif not z:
            globals_[n["id"]] = n
    return tabs, members, globals_


def flow_payload(flows, tid):
    """Monta o corpo de PUT /flow/{id} a partir de uma lista de nós."""
    tab = next(n for n in flows if n["type"] == "tab" and n["id"] == tid)
    nodes, configs = [], []
    for n in flows:
        if n.get("z") != tid:
            continue
        # config node local à aba: sem wires e sem x/y
        if "wires" not in n and "x" not in n:
            configs.append(n)
        else:
            nodes.append(n)
    payload = {"id": tid, "label": tab.get("label", ""), "nodes": nodes, "configs": configs}
    for k in ("disabled", "info", "env"):
        if k in tab:
            payload[k] = tab[k]
    return payload


def diff_report(local, server):
    ltabs, lmem, lglob = by_tab(local)
    stabs, smem, sglob = by_tab(server)
    lines = []
    changed_tabs = []
    for tid, tab in ltabs.items():
        label = tab.get("label", tid)
        if tid not in stabs:
            lines.append(f"  [+] aba só no local: {label} ({tid})")
            changed_tabs.append(tid)
            continue
        ln, sn = lmem[tid], smem[tid]
        diffs = []
        for nid in sorted(set(ln) | set(sn)):
            a, b = ln.get(nid), sn.get(nid)
            if a is None:
                diffs.append(f"      - só no servidor: {b.get('name') or b['type']} ({nid})")
            elif b is None:
                diffs.append(f"      + só no local:    {a.get('name') or a['type']} ({nid})")
            elif normalize(a) != normalize(b):
                diffs.append(f"      ~ diferente:      {a.get('name') or a['type']} ({nid})")
        if normalize(tab) != normalize(stabs[tid]):
            diffs.append("      ~ propriedades da aba (label/disabled/info)")
        if diffs:
            changed_tabs.append(tid)
            lines.append(f"  [~] {label} ({tid})")
            lines.extend(diffs)
    for tid, tab in stabs.items():
        if tid not in ltabs:
            lines.append(f"  [-] aba só no servidor: {tab.get('label', tid)} ({tid})")
    gdiff = [nid for nid in sorted(set(lglob) | set(sglob))
             if normalize(lglob.get(nid)) != normalize(sglob.get(nid))]
    if gdiff:
        lines.append("  [~] config nodes globais diferentes (NÃO sobem por --tab):")
        for nid in gdiff:
            n = lglob.get(nid) or sglob.get(nid)
            lines.append(f"      ~ {n.get('name') or n['type']} ({nid})")
    if not lines:
        lines.append("  (idênticos)")
    return "\n".join(lines), changed_tabs


# ------------------------------------------------------------------ comandos
def cmd_login(nr, _):
    print(f"[ok] autenticado em {nr.base} como {nr.user}")


def cmd_backup(nr, _):
    data = nr.get_flows()
    path = save_backup(data, "flows_server")
    print(f"[ok] backup do servidor (rev {data.get('rev')}): {path}")
    return data


def cmd_diff(nr, _):
    server = nr.get_flows()
    report, changed = diff_report(load_local(), server["flows"])
    print(f"flows.json local x servidor (rev {server.get('rev')}):")
    print(report)
    return changed


def cmd_pull(nr, _):
    server = nr.get_flows()
    if os.path.isfile(FLOWS_LOCAL):
        print(f"[backup] local: {save_backup(load_local(), 'flows_local_pre_pull')}")
    with open(FLOWS_LOCAL, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(server["flows"], fh, indent=4, ensure_ascii=False)
        fh.write("\n")
    print(f"[ok] {FLOWS_LOCAL} <- servidor (rev {server.get('rev')}, {len(server['flows'])} nós)")


def cmd_deploy(nr, args):
    local = load_local()
    if args.all:
        if not args.yes:
            sys.exit("[erro] deploy --all substitui TODOS os fluxos e config nodes; confirme com --yes")
        server = nr.get_flows()
        print(f"[backup] {save_backup(server, 'flows_server_pre_all')}")
        res = nr.put_flows(local, server.get("rev"))
        print(f"[ok] PUT /flows -> rev {res.get('rev') if res else '?'}")
        return
    if not args.tab:
        sys.exit("[erro] informe --tab ID [ID ...] ou --all --yes")
    tabs = {n["id"]: n for n in local if n["type"] == "tab"}
    for tid in args.tab:
        if tid not in tabs:
            sys.exit(f"[erro] aba {tid} não existe no flows.json local")
    server = nr.get_flows()
    print(f"[backup] {save_backup(server, 'flows_server_pre_tab')}")
    for tid in args.tab:
        label = tabs[tid].get("label", tid)
        try:
            current = nr.get_flow(tid)
            print(f"[backup] {save_backup(current, f'flow_{tid}_pre')}")
        except SystemExit as e:
            print(f"[aviso] GET /flow/{tid} falhou ({e}); a aba será criada")
        payload = flow_payload(local, tid)
        res = nr.put_flow(tid, payload)
        print(f"[ok] PUT /flow/{tid} ({label}): {len(payload['nodes'])} nós, "
              f"{len(payload['configs'])} configs -> {res}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("login")
    sub.add_parser("backup")
    sub.add_parser("diff")
    sub.add_parser("pull")
    dp = sub.add_parser("deploy")
    dp.add_argument("--tab", nargs="+", metavar="ID")
    dp.add_argument("--all", action="store_true")
    dp.add_argument("--yes", action="store_true")
    args = ap.parse_args()

    nr = NodeRed(load_credentials()).login()
    try:
        {"login": cmd_login, "backup": cmd_backup, "diff": cmd_diff,
         "pull": cmd_pull, "deploy": cmd_deploy}[args.cmd](nr, args)
    finally:
        nr.logout()


if __name__ == "__main__":
    main()
