#!/bin/bash

# Script para gerar certificado SSL self-signed para testes

echo "=========================================="
echo "  Gerador de Certificado SSL Self-Signed"
echo "=========================================="
echo ""

CERT_FILE="cert.pem"
KEY_FILE="key.pem"
DAYS=365

# Verificar se já existem certificados
if [ -f "$CERT_FILE" ] && [ -f "$KEY_FILE" ]; then
    echo "AVISO: Certificados já existem!"
    echo "  - $CERT_FILE"
    echo "  - $KEY_FILE"
    echo ""
    read -p "Deseja sobrescrever? (s/N): " resposta
    if [ "$resposta" != "s" ] && [ "$resposta" != "S" ]; then
        echo "Operação cancelada."
        exit 0
    fi
    echo ""
fi

echo "A gerar novo certificado SSL..."
echo ""

# Gerar chave privada e certificado
openssl req -x509 -newkey rsa:4096 -nodes \
    -keyout "$KEY_FILE" \
    -out "$CERT_FILE" \
    -days $DAYS \
    -subj "/C=PT/ST=Aveiro/L=Aveiro/O=WebServer/OU=Development/CN=localhost" \
    -addext "subjectAltName=DNS:localhost,DNS:*.localhost,IP:127.0.0.1"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Certificado gerado com sucesso!"
    echo ""
    echo "Ficheiros criados:"
    echo "  - $CERT_FILE (certificado público)"
    echo "  - $KEY_FILE (chave privada)"
    echo ""
    echo "Válido por: $DAYS dias"
    echo ""
    echo "Para usar no browser:"
    echo "  1. Aceda a https://localhost:8443/"
    echo "  2. Aceite o aviso de segurança (certificado self-signed)"
    echo "  3. No Chrome: clique em 'Advanced' → 'Proceed to localhost (unsafe)'"
    echo "  4. No Firefox: clique em 'Advanced' → 'Accept the Risk and Continue'"
    echo ""
    
    # Mostrar informações do certificado
    echo "Informações do certificado:"
    echo "----------------------------"
    openssl x509 -in "$CERT_FILE" -noout -subject -dates -fingerprint
    echo ""
    
    # Verificar permissões
    chmod 600 "$KEY_FILE"
    chmod 644 "$CERT_FILE"
    
    echo "✓ Permissões ajustadas:"
    echo "  - $KEY_FILE: 600 (apenas owner pode ler)"
    echo "  - $CERT_FILE: 644 (todos podem ler)"
    
else
    echo ""
    echo "✗ ERRO ao gerar certificado!"
    echo "Verifique se o OpenSSL está instalado:"
    echo "  sudo apt-get install openssl   # Debian/Ubuntu"
    echo "  sudo yum install openssl       # RedHat/CentOS"
    exit 1
fi

echo ""
echo "=========================================="
echo "  Certificado pronto para uso!"
echo "=========================================="