set -e
gcc main.c glad/src/glad.c \
    -Iglad/include \
    -o app \
    -lglfw -lGL -ldl -lm

./app