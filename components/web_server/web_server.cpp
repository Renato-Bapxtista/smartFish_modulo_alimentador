#include "web_server.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "usecases.h"

static const char *TAG = "web_server";

static const char index_html[] = R"rawliteral(
<!doctype html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>smartFit - Painel</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <header class="app-header">
    <h1>smartFit</h1>
    <button id="btn-refresh">Atualizar</button>
  </header>

  <main class="container">
    <section class="card status-card">
      <h2>Estado</h2>
      <div id="status">Carregando...</div>
    </section>

    <section class="card control-card">
      <h2>Controle</h2>
      <label>Porções:</label>
      <input id="input-amount" type="number" value="1" min="1" />
      <button id="btn-feed">Dar Ração</button>
    </section>

    <section class="card history-card">
      <h2>Histórico</h2>
      <ul id="history"></ul>
    </section>
  </main>

  <footer class="app-footer">Versão embarcada - sem câmera</footer>

  <script src="/app.js"></script>
</body>
</html>
)rawliteral";

static const char style_css[] = R"rawliteral(
:root{--bg:#f4f6f8;--card:#fff;--accent:#2b7be4;--text:#222}
*{box-sizing:border-box}
body{font-family:Inter,system-ui,Arial,sans-serif;background:var(--bg);color:var(--text);margin:0}
.app-header{display:flex;justify-content:space-between;align-items:center;padding:12px 16px;background:var(--card);box-shadow:0 1px 2px rgba(0,0,0,.05)}
.app-header h1{margin:0;font-size:1.1rem}
.container{display:grid;grid-template-columns:1fr;gap:12px;padding:16px}
.card{background:var(--card);padding:12px;border-radius:8px;box-shadow:0 1px 2px rgba(0,0,0,.04)}
.card h2{margin:0 0 8px 0;font-size:1rem}
#status{font-weight:600}
.control-card input{width:64px;padding:6px;margin-right:8px}
button{background:var(--accent);color:white;border:none;padding:8px 12px;border-radius:6px;cursor:pointer}
button:active{transform:translateY(1px)}
.app-footer{text-align:center;padding:10px;font-size:.85rem;color:#666}

@media(min-width:720px){
  .container{grid-template-columns:1fr 1fr;max-width:980px;margin:16px auto}
  .history-card{grid-column:1 / 3}
}
)rawliteral";

static const char app_js[] = R"rawliteral(
async function fetchStatus(){
  try{
    const r=await fetch('/status');
    if(!r.ok) throw new Error('HTTP '+r.status);
    const json=await r.json();
    document.getElementById('status').innerText = json.state || 'desconhecido';
    const hist=document.getElementById('history'); hist.innerHTML='';
    (json.history||[]).slice(0,10).forEach(it=>{
      const li=document.createElement('li'); li.innerText = `${it.time} — ${it.action}`; hist.appendChild(li);
    });
  }catch(e){document.getElementById('status').innerText='erro';}
}

document.getElementById('btn-refresh').addEventListener('click',fetchStatus);
document.getElementById('btn-feed').addEventListener('click',async ()=>{
  const amount = Number(document.getElementById('input-amount').value)||1;
  try{
    const r = await fetch('/feed',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({amount})});
    if(!r.ok) throw new Error('fail');
    await fetchStatus();
    alert('Comando enviado');
  }catch(e){alert('Erro ao enviar comando');}
});

// initial
fetchStatus();
)rawliteral";

static esp_err_t index_get_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t css_get_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/css");
  httpd_resp_send(req, style_css, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t js_get_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "application/javascript");
  httpd_resp_send(req, app_js, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// Simple JSON endpoints (stubs) — should be replaced by adapters calling core use-cases
static esp_err_t status_api_get(httpd_req_t *req){
  std::string s = Core::status_json();
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, s.c_str(), s.size());
  return ESP_OK;
}

static esp_err_t feed_api_post(httpd_req_t *req){
  char buf[128];
  int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
  if(ret <= 0) return ESP_FAIL;
  buf[ret]=0;
  // very small JSON parse: look for "amount"
  int amount = 1;
  const char *p = strstr(buf, "amount");
  if(p){
    // find digits
    while(*p && (*p<'0' || *p>'9')) p++;
    if(*p>='0' && *p<='9') amount = atoi(p);
  }
  FeedResult r = Core::feed(amount);
  std::string resp = std::string("{\"result\":\"") + (r.ok?"ok":"fail") + "\",\"message\":\"" + r.message + "\"}";
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, resp.c_str(), resp.size());
  return ESP_OK;
}

httpd_handle_t start_web_server(void){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;
  if (httpd_start(&server, &config) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
  }

  httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_get_handler, .user_ctx = NULL };
  httpd_register_uri_handler(server, &index_uri);

  httpd_uri_t css_uri = { .uri = "/style.css", .method = HTTP_GET, .handler = css_get_handler, .user_ctx = NULL };
  httpd_register_uri_handler(server, &css_uri);

  httpd_uri_t js_uri = { .uri = "/app.js", .method = HTTP_GET, .handler = js_get_handler, .user_ctx = NULL };
  httpd_register_uri_handler(server, &js_uri);

  httpd_uri_t status_uri = { .uri = "/status", .method = HTTP_GET, .handler = status_api_get, .user_ctx = NULL };
  httpd_register_uri_handler(server, &status_uri);

  httpd_uri_t feed_uri = { .uri = "/feed", .method = HTTP_POST, .handler = feed_api_post, .user_ctx = NULL };
  httpd_register_uri_handler(server, &feed_uri);

  ESP_LOGI(TAG, "HTTP server started");
  return server;
}

