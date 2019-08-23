FROM trzeci/emscripten-ubuntu:latest

WORKDIR /app

RUN apt-get update

COPY . .

RUN make all
