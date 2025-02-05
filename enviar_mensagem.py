import requests  # Importação necessária

bot_token = "8111874052:AAFn_63Sh0hJUuOP4IyzeBtxNOH2dQ2n9No"
chat_ids = ["207223980", "975571557"]  # IDs dos destinatários
mensagem = "SUPORTE: Prezados Usuários, estou ajustando para 65% a cisterna A e B para diminuir essa quantidade de mensagens enquanto fizemos a investigação desse comportamento e buscaremos uma solução."

for chat_id in chat_ids:
    url = f"https://api.telegram.org/bot{bot_token}/sendMessage?chat_id={chat_id}&text={mensagem}"
    response = requests.get(url)

    if response.status_code == 200:
        print(f"Mensagem enviada com sucesso para {chat_id}")
    else:
        print(f"Erro ao enviar mensagem para {chat_id}: {response.status_code} - {response.text}")
