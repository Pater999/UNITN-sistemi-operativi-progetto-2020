FROM ubuntu:bionic

RUN apt-get update && apt-get install -y \
    build-essential \
    nano && mkdir -p /progetto/src

WORKDIR /progetto

COPY makefile .
COPY interruttore.sh .

# Se lo script è stato scritto su windows si deve eseguire questo comando 
# per togliere i caratteri non validi dallo script bash
RUN sed -i -e 's/\r$//' interruttore.sh && chmod +x interruttore.sh

COPY ./src ./src