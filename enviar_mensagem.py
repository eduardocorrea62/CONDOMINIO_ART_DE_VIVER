import requests
import os  # Importa a biblioteca para pegar variáveis de ambiente

bot_token = os.getenv("BOT_TOKEN")  # Busca o token do GitHub Secrets
chat_ids = ["207223980", "975571557"]
mensagem = "SUPORTE: Ajustamos a cisterna A e B para reduzir oscilações."

for chat_id in chat_ids:
    url = f"https://api.telegram.org/bot{bot_token}/sendMessage?chat_id={chat_id}&text={mensagem}"
    response = requests.get(url)

    if response.status_code == 200:
        print(f"Mensagem enviada com sucesso para {chat_id}")
    else:
        print(f"Erro ao enviar mensagem para {chat_id}: {response.status_code} - {response.text}")
