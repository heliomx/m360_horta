# **Protocolo de Testes: MVP Manejo360 \- Irrigação Inteligente**

## **1\. Objetivo do Experimento**

Comparar a eficiência hídrica e a produtividade de dois canteiros de alface, avaliando a economia de água gerada pelo sistema **Manejo360** (baseado em sensores e demanda real) em relação ao **Manejo Tradicional** (irrigação por tempo fixo).

## **2\. Design Experimental**

* **Local:** \[Inserir Localização/Propriedade\]  
* **Duração Estimada:** 35 a 45 dias (desde o transplante até a colheita).  
* **Variedades:** Alface Crespa e Alface Roxa (proporção 50/50 em cada canteiro).  
* **Dimensões:** Dois canteiros de 1,20m x 7,00m (Área Total: 8,4 ![][image1] por canteiro).  
* **Espaçamento:** 0,25m x 0,25m (aproximadamente 134 plantas por canteiro).

### **Divisão dos Canteiros**

| Canteiro | Sistema de Controle | Ativação da Rega | Monitoramento de Água |
| :---- | :---- | :---- | :---- |
| **A (Manejo360)** | MANEJO360 (Rede de Sensores, atuadores \+ Microcontrolador) | Automática (conforme necessidade da planta) | Sensor de Vazão Digital |
| **B (Controle)** | Temporizador (Timer) \+ Válvula Solenoide | Fixa (2x ao dia: Manhã e Tarde) | Sensor de Vazão Digital |

## **3\. Preparação e Insumos (Padronização)**

Para garantir que a única variável seja a irrigação, ambos os canteiros devem seguir rigorosamente os mesmos procedimentos de preparo, conforme a *Planilha de Custos* e o *Manual da Embrapa*:

1. **Solo:** Calagem e adubação de base idênticas.  
2. **Adubação:** NPK e Adubo Orgânico aplicados na mesma data e quantidade.  
3. **Proteção:** Uso de mulching ou cobertura morta (se definido) em ambos ou em nenhum.  
4. **Fitossanidade:** Tratamento de pragas e doenças realizado simultaneamente em ambos os lados.

## **4\. Configuração dos Sistemas de Irrigação**

### **Canteiro A (Manejo360)**

* **Sensores:** Posicionados a 10 cm e 25 cm de profundidade (zona radicular ativa).  
* **Lógica:** O sistema deve ligar quando a tensão da água no solo atingir o limite crítico para alface (conforme Incaper, evitar que ultrapasse 20-30 kPa) e desligar ao atingir a capacidade de campo.

### **Canteiro B (Tradicional/Controle)**

* **Horários:** 07:30h (Manhã) e 16:30h (Tarde).  
* **Tempo de Rega:** Definido inicialmente conforme o costume local do agricultor (ex: 10-15 minutos por sessão), permanecendo fixo durante todo o ciclo, salvo condições climáticas extremas.

## **5\. Coleta de Dados e Monitoramento**

### **Diário (Automático)**

* **Consumo de Água (L):** Volume total gasto em cada canteiro (via sensores de vazão).  
* **Umidade do Solo:** Registro das leituras do Canteiro A para análise de comportamento.  
* **Contexto Climático:** Registro diário de chuva (mm) e temperatura.  
* **Log de Eventos (Canteiro A):** Timestamp de cada ativação, duração e volume por evento.  
* **Gráfico de Escada de Umidade:** Comparação entre a zona ideal (A) vs flutuações do timer (B).

### **Semanal (Manual/Visual)**

* **Saúde das Plantas:** Observação de sinais de estresse hídrico (murchamento) ou excesso (amarelamento).  
* **Contagem de Perdas:** Registro de eventuais plantas mortas em cada canteiro.

### **Final (Colheita)**

1. **Massa Fresca Total:** Peso total do canteiro A vs Canteiro B.  
2. **Peso Médio por Cabeça:** Pesagem individual de 10 amostras aleatórias de cada variedade (Crespa e Roxa) por canteiro.  
3. **Qualidade Visual:** Escala de 1 a 5 para aparência e tamanho comercial.

## **6\. Indicadores de Desempenho (KPIs)**

1. **Eficiência Hídrica (![][image2]):**  
   ![][image3]  
2. **Percentual de Economia de Água:**  
   ![][image4]  
3. **Custo de Irrigação por kg:** Cruzamento dos dados de vazão com o custo de energia/água da planilha de insumos.  
4. **Custo por kg produzido:** Total gasto em insumos e recursos / produção total.  
5. **Economia Projetada por Hectare:** Extrapolação dos dados para escala comercial.  
6. **Estimativa de Payback:** Quantos ciclos são necessários para o sistema se pagar.

## **7\. Métricas Proprietárias (Linguagem de Produto)**

O sucesso será medido através de índices exclusivos do Manejo360:

5. **Índice Manejo360 (![][image5]):** Razão entre a Eficiência Hídrica (kg/L) do sistema A e do sistema B.  
   * ![][image6]: Superioridade do produto confirmada.  
6. **Índice de Estabilidade Hídrica (![][image7]):** Consistência da umidade no solo A (menor oscilação \= menor estresse radicular).  
7. **Percentual de Economia Real:** Redução direta no volume de água (![][image8]) comparado ao 

## **8\. Cronograma de Atividades**

| Fase | Atividade | Responsável |
| :---- | :---- | :---- |
| **Semana 0** | Preparo do solo, instalação hidráulica e calibração dos sensores de vazão. | Equipe Técnica |
| **Dia 1** | Transplante das mudas e início do monitoramento. | Agricultor / Equipe |
| **Semana 1-6** | Monitoramento diário de vazão e manutenção fitossanitária. | Equipe / Sensores |
| **Dia Colheita** | Pesagem final, tabulação de dados e análise de resultados. | Equipe Técnica |

**Observações:** \- Em caso de chuva, o Canteiro A deve suspender a irrigação automaticamente. No Canteiro B, o temporizador deve ser desligado manualmente para não enviesar o teste de economia.

[image1]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABkAAAAYCAYAAAAPtVbGAAABWUlEQVR4Xu2SO0sDURCFNwQrsV5wn4WwoI2wCoKCjYWVYCepJOAfELGRIGJjbyUoiP9ERBtLq7TWxtZCQb8Jc+PNENYUa6HkwGH2njOP+9gg+EvIsuwRfsJ769UCGr943zfw3fdrgZwgz/MzXTZkPZRQN5IkWf71IXJVsGX12kDzK65tx+q1gQFtBqzId5qmq9YPMBfhRRiG07IuimKGog7cZ9lweRQfwus4jqNBcdB/hyV5eLiLv0dd1/eDsiynEO9I3JIHg6fSSDzisWjEeS1skjcrWhRFseuhdUN0Xh80uJUouxCTeOI8PZEMef6uGDQ98LVKkNzR2LU7YN22Gmjq4AWj/wzd3YOvyQnsEDmp1caGXtWm1dx1+hrs+dpYkH971O508LrVGLyh32++VwmSn+yQbMR70HzNacQjfuU5368EBR/w3GiX8NXXVO/JILhtvQkm+Kf4AsmMZdoXapxIAAAAAElFTkSuQmCC>

[image2]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACEAAAAYCAYAAAB0kZQKAAABtUlEQVR4Xu2Vu0oDURCGV7zUgti4bDZFqtgIKcRSsTAR1CewsrETvBUWFoKFIFqIjW8g+AIWFlpa+QBiYxEQUfESRKP+Q2b07L+XpLTYD4bdnf+fc+bknN14Xk5OBsVi8TQMw+9OgmuRW0A8kO8OUpfqdUTT0Z4QWzTMH2bkvBAEwVCaJmTVFgqFTdWrrDHdarxiwUibRNDaJucF5F+yan+BaUWMWPGMm8dWLTmexIGw0mFtYpc1QbXE2ghha68iRjzXMMG8PaOhUVc3kD+WWlz7WSuXy32qnbMWw7pFjCHGMfkaN5WG1WKiOVynpXlEVeNI9QmuY+w83GDybVwPELeSY2MSWvuBWKZYVa39OJh4XYxYyZSbR+7a7n3fH/D0tXNxzsMOa0LHTcD0zMZKpdKLXM3xvLu6gfyJLiB2HnQMaeKCtRjtui2VSoPQzzgvZNUivyEafq1J1iLY6UVcsmboJD2cF7Q27fvQSGswAn7GfTHy90E1Oe0ySeJWQJ9V/ZA1QbX0JvTd/kR8mZlC8nLiG/COUO1i2DpH8p9xj3gUr+m4f9WcaOJ5w5bsuWPk5OT8e34AFpS4WowoZVUAAAAASUVORK5CYII=>

[image3]: <data:image/png;base64,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>

[image4]: <data:image/png;base64,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>

[image5]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADwAAAAYCAYAAACmwZ5SAAADT0lEQVR4Xu1WSWhUQRCdGEXFBbdhZLaeDUdBEB3wIIgI4oLixYMgHjxpiEdBFD0IgqIiiuLJDTQeFDRRcYPgxUMIIWgQIuLJNUQxiVGJmEnGVzNVfyqVP4o3kf+g+L/ee11d3X+6k1AoQIAA/w2cc12IIURJRb/1JZPJh8Z30no0oBeVdyCRSKwUrVAoTALXyVovYrUeqwFtBWKQvOjhlNUJ0C6w/iaTySSt7gtpzvIW4kN0WU0ArRFxjuvVay2Xy82kxiRPp9NLuN5N7SOAa0P0qvxFKpXabzyjqLFW5SV41muPHybypE+toIFCs+BpYm/NzcGCWqD/9POAe0y8/uJ+9ZAf1lw0Gp3Hvvu1PATMvcly4wDTXi620Woa0I/H4/Ecnt9qFZWvx/U+WR2b1kAavnRYOPbaBRN3UXMYu1Dn7OnWnPDZbDZheQ+Oz4jlLeAZ5uddPz+4ZYilkUhkGjez23ossIFTyYuNahcOC1vH47dQjl/D5uqIKtjTWoM/Y3kPbBi3AAvx4LnHzw/uNT3R/FE/3QILS/Hcbw1/nXjU2Y73Q3jOJg+iR/vIA/2O5oRHPLC8QM5vhxU0wuHwdHiu0burfMmSvhGRN6v3YdIl9wMaPQbPJZ77gNaQv2S+z/DEneV0Aue3tIfA/CvLl4GJ95GBDrvVNOirYYELJKcxGLuD3vP5/Aynfr48oXe7/gnsH1J5B/e008fnbSR7WrRHeMQTy5fh/vL8qpyKXuX3H8Lz4klrrLp/D3i/0Bhs4C7KsYjLlOPsZo2P6toFP9Ie4RHnLV+GLVIL1sPj3iMO0sWj+BPsrVN2D9A+OD7rimvleuULCM8NnC8yPuLGLNjVuKWxeVstT6jnQc+soBGLxeaiwA3NyeSIJsOP6KYsZJzh3nGTDdpHl5bx0dhRlXfbWvgnZLnlPGCC01xkm9UE+Hs5Gfp3xD3NI+/xKVzH9QYN7wFa0f6Z4THeGWbuCmJEcr40aVPmC4df1hzmpgiH/CuiU3IhaWfo7PYhPiMGqJExplD5LLWD72cP/bPhNQDtCHYzL7mrbArVIS9NWkQjq0TXgPbRVRYpt/Fz6yGAb2adjgGd6cXWgz7WkIa5brtKr23WEyBAgAABAvxD+AU8GlTQbp/xCQAAAABJRU5ErkJggg==>

[image6]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAF8AAAAYCAYAAACcESEhAAAEHElEQVR4Xu1YW0hUURQdsyc96CWGOnN8DE1JESX0EVQU0YOinz6i6KOvEv0riKI+gl5kRFH0VRaUfRSUVvQC6adAQqUiMCKid2aSmhVGjtra3X3G7e6O945GfXQXbO7da61z7jn7Ps6ZCYUCBAgQIMB/CWPMI0QHokdEq/ZFIpFbyndYeySgx4W3LRwOz7daUVHRMHD1rDUhFsm2EtDmIdrJizEc0ToB2inWX+fn50e0/q+AMdVpzhW2UJrXsD7EI61ZQCtBnOD+0qUWjUbHUZFsnpeXN4v7uyR9BHA1iCaRP8nNzd2hPN3oY6nIe+BZLj1/E5jbfVEjz3oShrL5gRYkMKnx8FR4dYwBVEH/4eYBd4d4+Sa49Yd8j+SysrIms+9GMg8B116lOTfghs3V3J8ExrDFzzhowNt4Yiu1JgG9LCcnJ4rj12Qd26ea+2vWOm5gMWl4AzIsx15dfOLKJYe202TOngbJWb6goCCseYnCwsLh8L1E3EOapvXBwvgtvuFvquY14Onk4zU3P7g5iNmZmZmjuTCl2qOBmzmKvPS6Wg5FXsbt11COt2R1b4tesKc6CX9M80kwxDjr3gsaixYHCpNC8WmwnkbrwXGrmx/cKzqikAfcdA0UOZev/UbxF4hHPxtwvhvHCeRBNEofeaBflZzlETc17wVqg2hHn1O0liqMz+Lb732tFiQyMjLGwHOezo3zhPfInQXySnHe6XVhTPAgPKf52julhvwp8y2KJ+44p/TEUn5ZegjMP9O8X+Bmn0H7ON6EmVrzC+On+CjCdjLRQqU1CXqaUeypNqc2aLuRzmOx2FgjPjE8+cQuxQvs7xB5LY9pk4svMSH2VEmP5RF3NZ8q0Mc+6guL8wKtecH4Kb5J8XsvcprgOT7/bnm+EaSV9Lr7B7yfqQ1u5mbK+cmjHVGB8lG/uvi3pcfyiJOaTxXoo5THtU5rXjA+i99nQsmgPdzuHWKXXKiQH2Kv6w4C2nvDa4Pgqrm/X4snjis4n658xPUpvkmy20HB1mreL9B+L/e9WGt+YXwUP50v8lALEtnZ2ZMwmYuSs4VAVCi+q7+L2naKe0scrlEsfbTgKh+17RZ5g+6L9u+a8wu0KzfOetXnpg8Exqv4mOxRntB6rVlgPz4C+jfEdckjb3TpPI37a1d8AtDieuvIbRLffObOIrpszgs+3aDETgRv3ETmRloO+RdEvc39IOL8ddIqf3sMFuivjMb2W5/GeWLoW9+C+IRoQ8T7mEKJn8qt7KEfVoliQNuPpyxmc+PcIOqHvFSAOIqy0OoS0D4ap+B2V/NYewjgK1mnTxWtATO0B+NYQhqudcU4Y63RniSgB6UO8VzevMHCOL/sPxjnbabtMR2bDa+PAUKJvxdc16UAAQIECBAgQIAAA8VP4q6XxQ/W8rwAAAAASUVORK5CYII=>

[image7]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACsAAAAYCAYAAABjswTDAAACAklEQVR4Xu2WSyuEYRTHFcpCogyamnlmmimxsRjJwoqNWz6AL+ADkKRYUpTY2PkEtmIrNiIhJVsbFu53uf9PzhnHmWfGW1bq/dVpvOf/P+e5vE/Pq6goJOQf45zbQzwiPlRcenzWkzc8tYeIO+V5Rxyz1oA40/WI01QqVWP7ZBGjzVtkMJsnkO8p1OOX2qdCtZoSbrRjBU0sFmsmXyKRmLOakG/AZDLZxGPMWI1gzVv7g3g8PszmHqtpoC+RD/4qncfkO5VnX2sC8ou80EqrZTKZUh5/3Wo5wHQTZFW+1eN5Crvm5BmTSSg5i69WQH6M9Xar5VCokUZ8mE8rdrcDMRGkjpBaHKU+/HYjulQ8BO0j53XLChrsXgv7VjHJSfzOI26DDAJ/I9cuIwZVDKkj+Gsfem0jZERRr9U08KyQLxKJlEsOC6hFbkE918vfGsfnNZ1OV1hNzivmsWa1HNwfzisW2BaNRqu1R+uCr1ZAfpQ0OlZWy6FQIw37Xm1eoBsBA47bPMG1bzZPIH8fZHyimBvtWkFDO8i+aasJ+QaUu9n99X7Fbsyyud9qGvg2yee7I5Eb4B7eBSO/Qbrv80lXHtduWy0LxAP3dVYvEOeIK+d5xcidIN7c17ecmtog7QXxgEmXmdoj9z0GxbXjTy2/Kfpf4dJ9j/+MHnW6R0hISMg/5RO2P+g28/1dgwAAAABJRU5ErkJggg==>

[image8]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA0AAAAYCAYAAAAh8HdUAAAAmklEQVR4XmNgGOZAXl5+HhB/AuL/SPgjEPehq8UAMA3o4vgAI1TTWXQJnACoOBuqyQtdDicAKn5JqtPI8g9M0wl0cZyAkH+A4k7oYiDB1/icBpT7iC6G1z9A8RogdkQXZ4ZquoguoaysLIvVMDk5uX6opkA08RlQ8QtwQSBnMRD/AuK/QPwPqgCGQfw/QPxdWlpaBsmsUUBfAAA9ajx6d3mGNgAAAABJRU5ErkJggg==>