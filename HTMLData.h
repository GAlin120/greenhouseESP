const char HTTP_HEAD[] PROGMEM = "\
<!DOCTYPE html>\n\
<html>\n\
<head>\n\
<meta charset='utf-8' name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'/>\n\
<title>{v}</title>\n\
<style>\n\
.c{text-align:center;margin-top:20px;}\n\
div,input,a{padding:5px;font-size:1em;}\n\
input,a,p{width:95%;}\n\
body{text-align:center;font-family:verdana;}\n\
button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1rem;width:100%; margin-top:10px}\n\
.q{float:right;width:64px;text-align:right;}\n\
.l{background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==') no-repeat left center;background-size: 1em;}\n\
p{text-align:center;margin-top:0px;margin-bottom:0px;}\n\
</style>\n\
</head>\n\
<body>\n\
<div style='text-align:left;display:inline-block;min-width:260px;'>";

const char HTTP_SCRIPT_SSID[] PROGMEM = "\
<script>\n\
function c(l){\n\
 document.getElementById('s').value=l.innerText||l.textContent;\n\
 document.getElementById('p').focus();\n\
}\n\
</script>\n";

const char HTTP_FORM_PARAM[] PROGMEM = "<input id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}'><br>";
const char HTTP_END[] PROGMEM = "</div></body></html>";
