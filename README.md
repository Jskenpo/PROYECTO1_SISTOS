# Chat Server in C

Este proyecto consiste en un servidor de chat desarrollado en C para el curso de Sistemas Operativos. El servidor permite la conexión de múltiples clientes que pueden intercambiar mensajes entre sí, cambiar su estado y obtener información sobre otros usuarios conectados.

## Características

- Registro de nuevos usuarios con nombre de usuario único y dirección IP.
- Cambio de estado de los usuarios (activo, inactivo, ocupado).
- Envío de mensajes entre usuarios (broadcasting y mensajes directos).
- Consulta de la lista de usuarios conectados.
- Obtención de información detallada de un usuario en particular.

## Requisitos

- Compilador de C compatible con el estándar C99.
- Sistema operativo Linux (el servidor fue desarrollado y probado en Linux).

## Instrucciones de Uso

1. Clona el repositorio en tu máquina local.
2. Compila el servidor usando el comando `make server`.
3. Ejecuta el servidor especificando el puerto en el que deseas que escuche conexiones (por ejemplo, `./server 8888`).
4. Compila y ejecuta el cliente en otra terminal usando el comando `make client`.
5. Conecta el cliente al servidor especificando la dirección IP y el puerto del servidor.
6. ¡Comienza a chatear!

## Autor

Este proyecto fue desarrollado por José Pablo Santisteban y Diego Valdez como parte del curso de Sistemas Operativos de la Universidad del Valle de Guatemala.

## Contribuciones

Las contribuciones son bienvenidas. Si deseas contribuir a este proyecto, sigue los pasos:

1. Haz un fork del repositorio.
2. Crea una nueva rama para tu función o corrección de errores (`git checkout -b feature/nueva-funcion`).
3. Realiza tus cambios y haz commit de ellos (`git commit -am 'Agrega una nueva función'`).
4. Sube tus cambios a tu fork del repositorio (`git push origin feature/nueva-funcion`).
5. Haz un pull request para que tus cambios sean revisados y fusionados.

