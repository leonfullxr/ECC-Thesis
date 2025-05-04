#!/usr/bin/env bash
mkdir -p results
./bin/bench --algo RSA --bits 2048 --iters 50 >> results/rsa_2048.csv
./bin/bench --algo ECC --curve secp256k1 --iters 50 >> results/ecc_secp256k1.csv
