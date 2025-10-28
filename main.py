import requests
from flask import Flask, render_template, jsonify

app = Flask(__name__)

IP_FIWARE = "20.46.254.134"
DEVICE = "Hosp"
DEVICE_ID = "001"

URL_BASE_ENGAJAMENTO = f"http://{IP_FIWARE}:8666/STH/v1/contextEntities/type/{DEVICE}/id/urn:ngsi-ld:{DEVICE}:{DEVICE_ID}/attributes"
URL_BASE_SCORE = f"http://{IP_FIWARE}:1026/v2/entities/urn:ngsi-ld:{DEVICE}:{DEVICE_ID}/attrs"

URL_ENGAJAMENTO_A = f"{URL_BASE_ENGAJAMENTO}/engajamentoA?lastN=30"
URL_ENGAJAMENTO_B = f"{URL_BASE_ENGAJAMENTO}/engajamentoB?lastN=30"

URL_SCORE_A = f"{URL_BASE_SCORE}/scoreA"
URL_SCORE_B = f"{URL_BASE_SCORE}/scoreB"

headers = {
    "fiware-service": "smart",
    "fiware-servicepath": "/"
}

def pegaDados (url):
    response = requests.get(f"{url}", headers=headers)
    if response.status_code == 200:
        data = response.json()
        try:
            values = data
            return values
        except KeyError as error:
            print(f"KeyError: {error}")
            return []
    else:
        print(f"Erro ao acessar {url}: {response.status_code}")
        return []
    
@app.route("/dados")
def dados():
    data_score_A = pegaDados(URL_SCORE_A)
    data_score_B = pegaDados(URL_SCORE_B)

    data_engj_A = pegaDados(URL_ENGAJAMENTO_A)['contextResponses'][0]['contextElement']['attributes'][0]['values']
    data_engj_B = pegaDados(URL_ENGAJAMENTO_B)['contextResponses'][0]['contextElement']['attributes'][0]['values']

    score_A = data_score_A["value"]
    score_B = data_score_B["value"]
    engj_A = [float(entry.get('attrValue', 0)) for entry in data_engj_A]
    engj_B = [float(entry.get('attrValue', 0)) for entry in data_engj_B]
    timestamp = [entry.get('recvTime') for entry in data_engj_A]

    return jsonify(
        {
            "timestamp": timestamp,
            "score_A": score_A,
            "score_B": score_B,
            "engj_A": engj_A,
            "engj_B": engj_B
        }
    )

@app.route("/")
def home():
    return render_template("index.html")

if __name__ == '__main__':
    app.run(debug=True)