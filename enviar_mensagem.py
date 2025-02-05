import requests
import os

bot_token = os.getenv("BOT_TOKEN")  # Pega o token do GitHub Secrets

if not bot_token:
    print("ERRO: BOT_TOKEN não foi encontrado. Verifique os Secrets no GitHub.")
    exit(1)

chat_ids = ["207223980", "975571557", "7490200680", "7700038822", "7157576413"]
mensagem = "SUPORTE: Prezados usuários, está sendo ajustado os alertas de 75% para 65% das cisternas A e B para diminuir essa quantidade de mensagens enquanto fazemos a investigação dessas oscilações anormais de níveis."

print(f"Token lido: {bot_token[:10]}**********")  # Exibe só parte do token por segurança

for chat_id in chat_ids:
    url = f"https://api.telegram.org/bot{bot_token}/sendMessage?chat_id={chat_id}&text={mensagem}"
    response = requests.get(url)

    if response.status_code == 200:
        print(f"✅ Mensagem enviada com sucesso para {chat_id}")
    else:
        print(f"❌ Erro ao enviar mensagem para {chat_id}: {response.status_code} - {response.text}")
