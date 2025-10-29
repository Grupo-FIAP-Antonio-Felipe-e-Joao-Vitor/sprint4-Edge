# ------------------------------------------------------------
# Autores: (adicione aqui seus nomes e RMs)
# Projeto: Dashboard Flask - Integração com FIWARE
# Descrição: Este servidor Flask coleta dados de pontuação e engajamento
# de um dispositivo registrado no FIWARE (via Orion Context Broker e STH Comet),
# processa os dados e disponibiliza uma API JSON para exibição em dashboards.
# ------------------------------------------------------------

import requests
from flask import Flask, render_template, jsonify

# Inicializa a aplicação Flask
app = Flask(__name__)

# ---------------- Configurações do FIWARE ----------------
IP_FIWARE = "20.46.254.134"   # IP do servidor FIWARE
DEVICE = "Hosp"               # Tipo da entidade (definido no IoT Agent)
DEVICE_ID = "001"             # ID do dispositivo (usado no Orion Context Broker)

# URLs base para acessar os dados de engajamento (STH Comet) e pontuação (Orion)
URL_BASE_ENGAJAMENTO = f"http://{IP_FIWARE}:8666/STH/v1/contextEntities/type/{DEVICE}/id/urn:ngsi-ld:{DEVICE}:{DEVICE_ID}/attributes"
URL_BASE_SCORE = f"http://{IP_FIWARE}:1026/v2/entities/urn:ngsi-ld:{DEVICE}:{DEVICE_ID}/attrs"

# URLs específicas para cada atributo de interesse
URL_ENGAJAMENTO_A = f"{URL_BASE_ENGAJAMENTO}/engajamentoA?lastN=30"  # Últimos 30 registros de engajamento do time A
URL_ENGAJAMENTO_B = f"{URL_BASE_ENGAJAMENTO}/engajamentoB?lastN=30"  # Últimos 30 registros de engajamento do time B

URL_SCORE_A = f"{URL_BASE_SCORE}/scoreA"  # Atributo de pontuação do time A
URL_SCORE_B = f"{URL_BASE_SCORE}/scoreB"  # Atributo de pontuação do time B

# Cabeçalhos exigidos pelo FIWARE (service e servicepath)
headers = {
    "fiware-service": "smart",
    "fiware-servicepath": "/"
}

# ---------------- Funções auxiliares ----------------
def pegaDados(url):
    """
    Faz uma requisição HTTP GET para o endpoint informado e retorna o JSON.
    Retorna [] em caso de erro de conexão ou resposta inválida.
    """
    response = requests.get(url, headers=headers)
    if response.status_code == 200:
        data = response.json()
        try:
            # Retorna os dados diretamente se forem válidos
            return data
        except KeyError as error:
            print(f"KeyError ao acessar {url}: {error}")
            return []
    else:
        # Caso o FIWARE não responda corretamente
        print(f"Erro ao acessar {url}: Código {response.status_code}")
        return []

# ---------------- Rotas Flask ----------------

@app.route("/dados")
def dados():
    """
    Endpoint que retorna dados em formato JSON.
    - Busca os scores dos times A e B diretamente do Orion Context Broker
    - Busca os níveis de engajamento (histórico) via STH Comet
    - Retorna tudo estruturado em JSON para uso em gráficos/dashboards
    """

    # Requisições aos endpoints do FIWARE
    data_score_A = pegaDados(URL_SCORE_A)
    data_score_B = pegaDados(URL_SCORE_B)

    # Extrai histórico de engajamento dos dois times (últimos 30 registros)
    data_engj_A = pegaDados(URL_ENGAJAMENTO_A)['contextResponses'][0]['contextElement']['attributes'][0]['values']
    data_engj_B = pegaDados(URL_ENGAJAMENTO_B)['contextResponses'][0]['contextElement']['attributes'][0]['values']

    # Extrai valores atuais de pontuação
    score_A = data_score_A["value"]
    score_B = data_score_B["value"]

    # Converte as listas de engajamento em arrays numéricos
    engj_A = [float(entry.get('attrValue', 0)) for entry in data_engj_A]
    engj_B = [float(entry.get('attrValue', 0)) for entry in data_engj_B]

    # Extrai timestamps de cada leitura (para gráficos de linha)
    timestamp = [entry.get('recvTime') for entry in data_engj_A]

    # Retorna um JSON estruturado
    return jsonify({
        "timestamp": timestamp,
        "score_A": score_A,
        "score_B": score_B,
        "engj_A": engj_A,
        "engj_B": engj_B
    })

@app.route("/")
def home():
    """
    Página inicial da aplicação Flask.
    Renderiza o arquivo HTML 'index.html' localizado na pasta 'templates'.
    """
    return render_template("index.html")

# ---------------- Execução do servidor ----------------
if __name__ == '__main__':
    # Inicia o servidor Flask em modo debug, acessível em qualquer IP da rede
    app.run(
        debug=True,
        host="0.0.0.0",  # Permite acesso externo (outros dispositivos da rede)
        port=5000        # Porta de execução
    )
