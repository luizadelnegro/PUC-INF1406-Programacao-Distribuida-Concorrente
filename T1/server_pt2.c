// único processo/thread servidor utilizando a primitiva select.
//O serviço consiste em fornecer arquivos em chunks de 64 bytes. O servidor conhece o conteúdo de 10 arquivos de tamanhos variados. O protocolo deve ser o seguinte. Ao se conectar, o cliente envia ao servidor um número aleatório de 0 a 9, como um inteiro de 1 byte. O servidor responde com a string "ready\n" a uma mensagem correta e com "not ready\n" caso a mensagem do cliente seja diferente disso. Nesse segundo caso, o servidor fecha a conexão imediatamente. Caso o servidor responda "ready", o cliente então entra em um loop de mensagens com a string "nextchunk\n", e o servidor responde com os próximos 64 bytes do arquivo selecionado. Quando o servidor detectar que o arquivo acabou, deve fechar a conexão com o cliente.

//O servidor deve usar select para ficar escutando no socket mestre, a espera de conexões, e nos sockets dos clientes que já estabeleceram conexões. Um máximo de 10 clientes simultâneos deve poder ser atendido. Caso o servidor já esteja com 10 conexões estabelecidas, ele deve parar de escutar no socket mestre até que alguma das conexões com cliente seja fechada.

//Para evitar do servidor ficar pendurado, a espera de clientes que não terminem a leitura do arquivo, os seguintes mecanismos devem ser implementados:

