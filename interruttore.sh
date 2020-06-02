#!/bin/bash
interruttoreNumero="$1"

pidProcessoT1=$(pgrep -l 'T1_I')
pidProcessoT2=$(pgrep -l 'T2_I')
pidProcessoT3=$(pgrep -l 'T3_I')
pidProcessoT4=$(pgrep -l 'T4_I')

IFS=' '
if [ -z "$interruttoreNumero" ]
then
    echo "Devi passare il numero dell'interruttore da tirare! Usa ./interruttore.sh <numero_interruttore>"
else 
    if [ $1 = "1" ]
    then
        read -ra PIDT1 <<< "$pidProcessoT1"
        if [ -z ${PIDT1[0]} ]
        then
            echo "L'interruttore T1 non è presente oppure è un pulsante!"
        else
            echo "Premo l'interruttore T1"
            kill -10 ${PIDT1[0]}
        fi
    elif [ $1 = "2" ]
    then
        read -ra PIDT2 <<< "$pidProcessoT2"
        if [ -z ${PIDT2[0]} ]
        then
            echo "L'interruttore T2 non è presente oppure è un pulsante!"
        else
            echo "Premo l'interruttore T2"
            kill -10 ${PIDT2[0]}
        fi
    elif [ $1 = "3" ]
    then
        read -ra PIDT3 <<< "$pidProcessoT3"
        if [ -z ${PIDT3[0]} ]
        then
            echo "L'interruttore T3 non è presente oppure è un pulsante!"
        else
            echo "Premo l'interruttore T3"
            kill -10 ${PIDT3[0]}
        fi
    elif [ $1 = "4" ]
    then
        read -ra PIDT4 <<< "$pidProcessoT4"
        if [ -z ${PIDT4[0]} ]
        then
            echo "L'interruttore T4 non è presente oppure è un pulsante!"
        else
            echo "Premo l'interruttore T4"
            kill -10 ${PIDT4[0]}
        fi
    else
        echo "Esistono solo 4 interruttori. <numero_interruttore> deve essere un intero da 1 a 4."
    fi
fi