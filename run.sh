set -e
gcc main.c glad.c -I. -o opengl_app -lglfw -lGL -ldl -lm
./opengl_app