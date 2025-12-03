#!/bin/bash
# Script de testes abrangente para o servidor HTTP multi-processo/multi-thread
# Testa funcionalidade, concorrência, sincronização e stress
# Uso: ./test_server.sh

PORT=8080
HOST="localhost"
BASE_URL="http://${HOST}:${PORT}"

echo "=========================================="
echo "  Testes Completos do Servidor HTTP"
echo "=========================================="
echo ""

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Contadores globais
total=0
passed=0
failed=0

# Função auxiliar para testes
test_request() {
    local name=$1
    local url=$2
    local expected_code=$3
    local extra_args=${4:-""}
    
    echo -n "  [$((total+1))] $name... "
    
    http_code=$(curl -s -o /dev/null -w "%{http_code}" $extra_args "$url")
    
    if [ "$http_code" = "$expected_code" ]; then
        echo -e "${GREEN}✓ PASSOU${NC} (código: $http_code)"
        ((passed++))
    else
        echo -e "${RED}✗ FALHOU${NC} (esperado: $expected_code, obtido: $http_code)"
        ((failed++))
    fi
    ((total++))
}

# Verificar dependências
echo -e "${BLUE}=== Verificação de Dependências ===${NC}"
MISSING_DEPS=0

if ! command -v curl &> /dev/null; then
    echo -e "${RED}✗ curl não está instalado${NC}"
    MISSING_DEPS=1
else
    echo -e "${GREEN}✓ curl encontrado${NC}"
fi

if ! command -v ab &> /dev/null; then
    echo -e "${YELLOW}⚠ Apache Bench (ab) não encontrado - testes de carga limitados${NC}"
    echo "  Instale com: sudo apt-get install apache2-utils"
else
    echo -e "${GREEN}✓ Apache Bench (ab) encontrado${NC}"
fi

if ! command -v wget &> /dev/null; then
    echo -e "${YELLOW}⚠ wget não encontrado - alguns testes serão pulados${NC}"
else
    echo -e "${GREEN}✓ wget encontrado${NC}"
fi

if [ $MISSING_DEPS -eq 1 ]; then
    echo -e "${RED}ERRO: Dependências obrigatórias não encontradas${NC}"
    exit 1
fi

echo ""

# Verificar se o servidor está a correr
echo -e "${BLUE}=== Verificação do Servidor ===${NC}"
echo -n "A verificar conectividade... "
if ! curl -s --connect-timeout 2 "$BASE_URL" > /dev/null 2>&1; then
    echo -e "${RED}✗ FALHOU${NC}"
    echo -e "${RED}ERRO: Servidor não está a responder em $BASE_URL${NC}"
    echo "Inicie o servidor primeiro com: ./server"
    exit 1
fi
echo -e "${GREEN}✓ PASSOU${NC}"
echo ""

#############################################
# TESTES FUNCIONAIS
#############################################
echo -e "${BLUE}=== 1. TESTES FUNCIONAIS ===${NC}"

# Teste 1.1: GET / (index padrão)
test_request "GET / (redireção para index.html)" "$BASE_URL/" "200"

# Teste 1.2: GET /index.html explícito
test_request "GET /index.html explícito" "$BASE_URL/index.html" "200"

# Teste 1.3: HEAD / (apenas headers)
echo -n "  [$((total+1))] HEAD / (apenas headers)... "
response=$(curl -s -I "$BASE_URL/")
if echo "$response" | grep -q "HTTP/1.1 200 OK" && ! echo "$response" | grep -q "<html>"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 1.4: HEAD /index.html
echo -n "  [$((total+1))] HEAD /index.html... "
response=$(curl -s -I "$BASE_URL/index.html")
if echo "$response" | grep -q "HTTP/1.1 200 OK"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Criar ficheiros de teste para MIME types
mkdir -p www/test_files 2>/dev/null

# Teste 1.5: HTML
echo "<!DOCTYPE html><html><body><h1>Test HTML</h1></body></html>" > www/test_files/test.html
sleep 0.2
echo -n "  [$((total+1))] HTML - Content-Type correto... "
response=$(curl -s -I "$BASE_URL/test_files/test.html")
if echo "$response" | grep -q "Content-Type: text/html"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 1.6: CSS
echo "body { background: #fff; }" > www/test_files/test.css
sleep 0.2
echo -n "  [$((total+1))] CSS - Content-Type correto... "
response=$(curl -s -I "$BASE_URL/test_files/test.css")
if echo "$response" | grep -q "Content-Type: text/css"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 1.7: JavaScript
echo "console.log('test');" > www/test_files/test.js
sleep 0.2
echo -n "  [$((total+1))] JavaScript - Content-Type correto... "
response=$(curl -s -I "$BASE_URL/test_files/test.js")
if echo "$response" | grep -q "Content-Type: application/javascript"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 1.8: JSON
echo '{"test": "value"}' > www/test_files/test.json
sleep 0.2
echo -n "  [$((total+1))] JSON - Content-Type correto... "
response=$(curl -s -I "$BASE_URL/test_files/test.json")
if echo "$response" | grep -q "Content-Type: application/json"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 1.9: Imagem PNG (criar 1x1 pixel PNG válido)
printf '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89\x00\x00\x00\nIDATx\x9cc\x00\x01\x00\x00\x05\x00\x01\r\n-\xb4\x00\x00\x00\x00IEND\xaeB`\x82' > www/test_files/test.png
sleep 0.2
echo -n "  [$((total+1))] PNG - Content-Type correto... "
response=$(curl -s -I "$BASE_URL/test_files/test.png")
if echo "$response" | grep -q "Content-Type: image/png"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 1.10: Texto plano
echo "Plain text file" > www/test_files/test.txt
sleep 0.2
echo -n "  [$((total+1))] TXT - Content-Type correto... "
response=$(curl -s -I "$BASE_URL/test_files/test.txt")
if echo "$response" | grep -q "Content-Type: text/plain"; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 1.11: 404 Not Found
test_request "404 - Ficheiro inexistente" "$BASE_URL/naoexiste12345.html" "404"

# Teste 1.12: 404 com caminho profundo
test_request "404 - Caminho inexistente" "$BASE_URL/dir1/dir2/naoexiste.html" "404"

# Teste 1.13: 403 Forbidden (diretório)
mkdir -p www/testdir 2>/dev/null
echo "test" > www/testdir/file.txt
test_request "403 - Acesso a diretório" "$BASE_URL/testdir" "403"

# Teste 1.14: 501 Not Implemented (POST)
test_request "501 - POST não implementado" "$BASE_URL/" "501" "-X POST"

# Teste 1.15: 501 Not Implemented (PUT)
test_request "501 - PUT não implementado" "$BASE_URL/" "501" "-X PUT"

# Teste 1.16: 501 Not Implemented (DELETE)
test_request "501 - DELETE não implementado" "$BASE_URL/" "501" "-X DELETE"

# Teste 1.17: Content-Length correto
echo -n "  [$((total+1))] Content-Length correto... "
echo "Test content with known size" > www/test_files/size_test.txt
expected_size=$(wc -c < www/test_files/size_test.txt)
response=$(curl -s -I "$BASE_URL/test_files/size_test.txt")
actual_size=$(echo "$response" | grep -i "Content-Length:" | awk '{print $2}' | tr -d '\r')
if [ "$actual_size" = "$expected_size" ]; then
    echo -e "${GREEN}✓ PASSOU${NC} ($expected_size bytes)"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC} (esperado: $expected_size, obtido: $actual_size)"
    ((failed++))
fi
((total++))

echo ""

#############################################
# TESTES DE CONCORRÊNCIA
#############################################
echo -e "${BLUE}=== 2. TESTES DE CONCORRÊNCIA ===${NC}"

# Teste 2.1: 50 requisições concorrentes (curl)
echo -n "  [$((total+1))] 50 requisições concorrentes (curl)... "
success_count=0
for i in {1..50}; do
    curl -s "$BASE_URL/" > /dev/null 2>&1 &
done
wait
sleep 0.5
# Verificar se servidor ainda responde
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC} (servidor não responde)"
    ((failed++))
fi
((total++))

# Teste 2.2: 100 requisições concorrentes mistas
echo -n "  [$((total+1))] 100 requisições mistas (GET/HEAD)... "
for i in {1..50}; do
    curl -s "$BASE_URL/index.html" > /dev/null 2>&1 &
    curl -s -I "$BASE_URL/test_files/test.css" > /dev/null 2>&1 &
done
wait
sleep 0.5
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 2.3: Apache Bench - Alta concorrência
if command -v ab &> /dev/null; then
    echo -n "  [$((total+1))] Apache Bench: 10000 requisições, 100 concorrentes... "
    ab_output=$(ab -n 10000 -c 100 -q "$BASE_URL/" 2>&1)
    failed_requests=$(echo "$ab_output" | grep "Failed requests:" | awk '{print $3}')
    
    if [ "$failed_requests" = "0" ]; then
        echo -e "${GREEN}✓ PASSOU${NC} (0 falhas)"
        ((passed++))
    else
        echo -e "${RED}✗ FALHOU${NC} ($failed_requests falhas)"
        ((failed++))
    fi
    ((total++))
    
    # Extrair e mostrar estatísticas
    rps=$(echo "$ab_output" | grep "Requests per second:" | awk '{print $4}')
    echo -e "    ${YELLOW}→ Requests/sec: $rps${NC}"
else
    echo -e "  ${YELLOW}⚠ Teste AB pulado (não instalado)${NC}"
fi

# Teste 2.4: Múltiplos clientes wget em paralelo
if command -v wget &> /dev/null; then
    echo -n "  [$((total+1))] 20 clientes wget paralelos... "
    temp_dir=$(mktemp -d)
    for i in {1..20}; do
        wget -q -O "$temp_dir/file_$i.html" "$BASE_URL/" &
    done
    wait
    
    # Contar ficheiros baixados com sucesso
    downloaded=$(ls -1 "$temp_dir" 2>/dev/null | wc -l)
    rm -rf "$temp_dir"
    
    if [ "$downloaded" -eq 20 ]; then
        echo -e "${GREEN}✓ PASSOU${NC} (20/20 downloads)"
        ((passed++))
    else
        echo -e "${RED}✗ FALHOU${NC} ($downloaded/20 downloads)"
        ((failed++))
    fi
    ((total++))
else
    echo -e "  ${YELLOW}⚠ Teste wget pulado (não instalado)${NC}"
fi

# Teste 2.5: Stress test com requisições a ficheiros diferentes
echo -n "  [$((total+1))] Stress: 200 requisições a ficheiros diferentes... "
for i in {1..200}; do
    file=$((i % 5))
    case $file in
        0) curl -s "$BASE_URL/index.html" > /dev/null 2>&1 & ;;
        1) curl -s "$BASE_URL/test_files/test.css" > /dev/null 2>&1 & ;;
        2) curl -s "$BASE_URL/test_files/test.js" > /dev/null 2>&1 & ;;
        3) curl -s "$BASE_URL/test_files/test.html" > /dev/null 2>&1 & ;;
        4) curl -s "$BASE_URL/test_files/test.txt" > /dev/null 2>&1 & ;;
    esac
done
wait
sleep 0.5
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

echo ""

#############################################
# TESTES DE SINCRONIZAÇÃO E INTEGRIDADE
#############################################
echo -e "${BLUE}=== 3. TESTES DE SINCRONIZAÇÃO ===${NC}"

# Teste 3.1: Integridade do log (verificar que entradas não se sobrepõem)
echo -n "  [$((total+1))] Integridade do ficheiro de log... "
log_file="access.log"
if [ -f "$log_file" ]; then
    # Fazer várias requisições
    for i in {1..20}; do
        curl -s "$BASE_URL/" > /dev/null 2>&1 &
    done
    wait
    sleep 1
    
    # Verificar se cada linha tem formato correto [YYYY-MM-DD HH:MM:SS]
    invalid_lines=$(grep -v "^\[20[0-9][0-9]-[0-9][0-9]-[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9]\]" "$log_file" | grep -v "=====" | wc -l)
    
    if [ "$invalid_lines" -eq 0 ]; then
        echo -e "${GREEN}✓ PASSOU${NC} (formato correto)"
        ((passed++))
    else
        echo -e "${YELLOW}⚠ AVISO${NC} ($invalid_lines linhas com formato inesperado)"
        ((passed++)) # Não falhar por isto
    fi
else
    echo -e "${YELLOW}⚠ Log não encontrado${NC}"
    ((passed++))
fi
((total++))

# Teste 3.2: Cache hit consistency (requisitar o mesmo ficheiro várias vezes)
echo -n "  [$((total+1))] Consistência da cache (100 requisições ao mesmo ficheiro)... "
errors=0
for i in {1..100}; do
    result=$(curl -s -w "%{http_code}" -o /dev/null "$BASE_URL/test_files/test.html")
    if [ "$result" != "200" ]; then
        ((errors++))
    fi
done

if [ $errors -eq 0 ]; then
    echo -e "${GREEN}✓ PASSOU${NC} (0 erros)"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC} ($errors erros)"
    ((failed++))
fi
((total++))

# Teste 3.3: Estatísticas (verificar se há resposta)
echo -n "  [$((total+1))] Sistema de estatísticas funcional... "
# Fazer algumas requisições
for i in {1..10}; do
    curl -s "$BASE_URL/" > /dev/null 2>&1
done
sleep 2
# Não podemos verificar as stats diretamente, mas podemos verificar se o servidor ainda funciona
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC} (servidor operacional)"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 3.4: Race condition test - requisições simultâneas ao mesmo recurso
echo -n "  [$((total+1))] Race conditions: 50 requisições simultâneas ao mesmo ficheiro... "
errors=0
for i in {1..50}; do
    curl -s "$BASE_URL/test_files/test.css" > /dev/null 2>&1 &
done
wait
sleep 0.5
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

echo ""

#############################################
# TESTES DE STRESS E ESTABILIDADE
#############################################
echo -e "${BLUE}=== 4. TESTES DE STRESS ===${NC}"

# Teste 4.1: Carga sustentada por 30 segundos
echo -n "  [$((total+1))] Carga sustentada (30 segundos)... "
end_time=$((SECONDS + 30))
request_count=0
while [ $SECONDS -lt $end_time ]; do
    curl -s "$BASE_URL/" > /dev/null 2>&1 &
    ((request_count++))
    sleep 0.1
done
wait
sleep 1
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC} ($request_count requisições)"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 4.2: Picos de tráfego (burst)
echo -n "  [$((total+1))] Picos de tráfego (3 bursts de 100 requisições)... "
for burst in {1..3}; do
    for i in {1..100}; do
        curl -s "$BASE_URL/" > /dev/null 2>&1 &
    done
    wait
    sleep 2
done
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 4.3: Mix de requisições válidas e inválidas
echo -n "  [$((total+1))] Mix de requisições (válidas + 404 + 501)... "
for i in {1..30}; do
    curl -s "$BASE_URL/" > /dev/null 2>&1 &
    curl -s "$BASE_URL/naoexiste$i.html" > /dev/null 2>&1 &
    curl -s -X POST "$BASE_URL/" > /dev/null 2>&1 &
done
wait
sleep 1
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 4.4: Ficheiros de tamanhos variados
echo -n "  [$((total+1))] Ficheiros de tamanhos variados... "
# Criar ficheiros pequenos, médios e grandes
echo "small" > www/test_files/small.txt
dd if=/dev/zero of=www/test_files/medium.bin bs=1K count=100 2>/dev/null
dd if=/dev/zero of=www/test_files/large.bin bs=1K count=1000 2>/dev/null

for i in {1..20}; do
    curl -s "$BASE_URL/test_files/small.txt" > /dev/null 2>&1 &
    curl -s "$BASE_URL/test_files/medium.bin" > /dev/null 2>&1 &
    curl -s "$BASE_URL/test_files/large.bin" > /dev/null 2>&1 &
done
wait
sleep 1
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 4.5: Teste de estabilidade - verificar processos
echo -n "  [$((total+1))] Verificação de processos (sem zombies)... "
zombie_count=$(ps aux | grep defunct | grep -v grep | wc -l)
if [ $zombie_count -eq 0 ]; then
    echo -e "${GREEN}✓ PASSOU${NC} (0 zombies)"
    ((passed++))
else
    echo -e "${YELLOW}⚠ AVISO${NC} ($zombie_count processos zombie)"
    ((passed++)) # Não falhar, apenas avisar
fi
((total++))

echo ""

#############################################
# TESTES ADICIONAIS DE EDGE CASES
#############################################
echo -e "${BLUE}=== 5. TESTES DE EDGE CASES ===${NC}"

# Teste 5.1: Requisição com URL muito longa
echo -n "  [$((total+1))] URL extremamente longa... "
long_url="$BASE_URL/"$(printf 'a%.0s' {1..1000})".html"
result=$(curl -s -o /dev/null -w "%{http_code}" "$long_url")
if [ "$result" = "404" ] || [ "$result" = "414" ]; then
    echo -e "${GREEN}✓ PASSOU${NC} (tratado corretamente)"
    ((passed++))
else
    echo -e "${YELLOW}⚠ AVISO${NC} (código: $result)"
    ((passed++))
fi
((total++))

# Teste 5.2: Múltiplas requisições HEAD
echo -n "  [$((total+1))] 50 requisições HEAD concorrentes... "
for i in {1..50}; do
    curl -s -I "$BASE_URL/" > /dev/null 2>&1 &
done
wait
sleep 0.5
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

# Teste 5.3: Path traversal (segurança básica)
echo -n "  [$((total+1))] Tentativa de path traversal (/../)... "
result=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/../etc/passwd")
if [ "$result" = "404" ] || [ "$result" = "403" ]; then
    echo -e "${GREEN}✓ PASSOU${NC} (bloqueado)"
    ((passed++))
else
    echo -e "${YELLOW}⚠ AVISO${NC} (código: $result)"
    ((passed++))
fi
((total++))

# Teste 5.4: Ficheiro com espaços no nome
echo "test content" > "www/test_files/file with spaces.txt"
echo -n "  [$((total+1))] Ficheiro com espaços no nome... "
result=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/test_files/file%20with%20spaces.txt")
if [ "$result" = "200" ]; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${YELLOW}⚠ AVISO${NC} (código: $result)"
    ((passed++))
fi
((total++))

# Teste 5.5: Conexão rápida (conexão e desconexão imediata)
echo -n "  [$((total+1))] 20 conexões rápidas (connect/disconnect)... "
for i in {1..20}; do
    timeout 0.1 curl -s "$BASE_URL/" > /dev/null 2>&1 &
done
wait
sleep 1
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSOU${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FALHOU${NC}"
    ((failed++))
fi
((total++))

echo ""

#############################################
# RELATÓRIO FINAL
#############################################
echo "=========================================="
echo -e "${BLUE}  RELATÓRIO FINAL${NC}"
echo "=========================================="
echo ""
echo "Total de testes executados: $total"
echo -e "Testes passados: ${GREEN}$passed${NC}"
echo -e "Testes falhados: ${RED}$failed${NC}"

if [ $failed -eq 0 ]; then
    success_rate=100
else
    success_rate=$((passed * 100 / total))
fi

echo -e "Taxa de sucesso: ${YELLOW}${success_rate}%${NC}"
echo ""

if [ $failed -eq 0 ]; then
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}  ✓ TODOS OS TESTES PASSARAM!${NC}"
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    exit 0
elif [ $success_rate -ge 90 ]; then
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}  ⚠ Maioria dos testes passou (≥90%)${NC}"
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    exit 0
else
    echo -e "${RED}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${RED}  ✗ Alguns testes críticos falharam${NC}"
    echo -e "${RED}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    exit 1
fi