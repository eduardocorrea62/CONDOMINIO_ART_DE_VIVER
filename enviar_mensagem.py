Python 3.13.1 (tags/v3.13.1:0671451, Dec  3 2024, 19:06:28) [MSC v.1942 64 bit (AMD64)] on win32
Type "help", "copyright", "credits" or "license()" for more information.
>>> import requests
... 
... bot_token = "8111874052:AAFn_63Sh0hJUuOP4IyzeBtxNOH2dQ2n9No"
... chat_ids = ["207223980", "975571557"]  # Adicione mais IDs conforme necessário
... mensagem = "SUPORTE: Prezados Usuários, estou ajustando para 65% a cisterna A e B para diminuir essa quantidade de mensagens enquanto fizemos a investigação desse comportamento e buscaremos uma solução."
... 
... for chat_id in chat_ids:
...     url = f"https://api.telegram.org/bot{bot_token}/sendMessage?chat_id={chat_id}&text={mensagem}"
...     response = requests.get(url)
