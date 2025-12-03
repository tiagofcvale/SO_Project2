#!/bin/bash

# Script de testes para o servidor HTTP
# Uso: ./test_server.sh

PORT=8080
HOST="localhost"
BASE_URL="http://${HOST}:${PORT}"

echo "=================================="
echo "  Testes do Servidor HTTP"
echo "=================================="
echo ""

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Função auxiliar para testes
test_request() {
    local name=$1
    local url=$2
    local expected_code=$3
    
    echo -n "Teste: $name... "
    
    http_code=$(curl -s -o /dev/null -w "%{http_code}" "$url")
    
    if [ "$http_code" = "$expected_code" ]; then
        echo -e "${GREEN}✓ PASSOU${NC} (código: $http_code)"
        return 0
    else
        echo -e "${RED}✗ FALHOU${NC} (esperado: $expected_code, obtido: $http_code)"
        return 1
    fi
}

# Verificar se curl está instalado
if ! command -v curl &> /dev/null; then
    echo -e "${RED}ERRO: curl não está instalado${NC}"
    echo "Instale com: sudo apt-get install curl"
    exit 1
fi

# Verificar se o servidor está a correr
echo "A verificar se o servidor está ativo..."
if ! curl -s --connect-timeout 2 "$BASE_URL" > /dev/null 2>&1; then
    echo -e "${RED}ERRO: Servidor não está a responder em $BASE_URL${NC}"
    echo "Inicie o servidor primeiro com: ./server"
    exit 1
fi
echo -e "${GREEN}Servidor está ativo!${NC}"
echo ""

# Contador de testes
total=0
passed=0

# Teste 1: GET /
test_request "GET /" "$BASE_URL/" "200"
((total++)); [ $? -eq 0 ] && ((passed++))

# Teste 2: GET /index.html
test_request "GET /index.html" "$BASE_URL/index.html" "200"
((total++)); [ $? -eq 0 ] && ((passed++))

# Teste 3: GET /test.html
test_request "GET /test.html" "$BASE_URL/test.html" "200"
((total++)); [ $? -eq 0 ] && ((passed++))

# Teste 4: HEAD /
echo -n "Teste: HEAD /... "
response=$(curl -s -I "$BASE_URL/")
if echo "$response" | grep -q "HTTP/1.1 200 OK"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
fi
((total++))

# Teste 5: 404 Not Found
test_request "404 Not Found" "$BASE_URL/naoexiste.html" "404"
((total++)); [ $? -eq 0 ] && ((passed++))

# Teste 6: Ficheiro CSS (MIME type)
echo -n "Teste: Criar e testar CSS... "
echo "body { color: red; }" > www/test.css
sleep 0.5
response=$(curl -s -I "$BASE_URL/test.css")
if echo "$response" | grep -q "Content-Type: text/css"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
fi
((total++))

# Teste 7: Ficheiro JavaScript (MIME type)
echo -n "Teste: Criar e testar JS... "
echo "console.log('test');" > www/test.js
sleep 0.5
response=$(curl -s -I "$BASE_URL/test.js")
if echo "$response" | grep -q "Content-Type: application/javascript"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
fi
((total++))

# Teste 8: Requisições concorrentes
echo -n "Teste: 20 requisições concorrentes... "
success=0
for i in {1..20}; do
    curl -s "$BASE_URL/" > /dev/null 2>&1 &
done
wait
# Verificar se o servidor ainda responde
if curl -s --connect-timeout 2 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
fi
((total++))

# Teste 9: Método POST não implementado (501)
test_request "POST não implementado" "$BASE_URL/" "501" -X POST
((total++)); [ $? -eq 0 ] && ((passed++))

echo ""
echo "=================================="
echo "  Resultados"
echo "=================================="
echo "Total de testes: $total"
echo -e "Passaram: ${GREEN}$passed${NC}"
echo -e "Falharam: ${RED}$((total - passed))${NC}"

if [ $passed -eq $total ]; then
    echo -e "${GREEN}Todos os testes passaram!${NC}"
    exit 0
else
    echo -e "${YELLOW}Alguns testes falharam.${NC}"
    exit 1
fi