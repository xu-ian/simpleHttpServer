function send(method, url, data, callback) {
  const xhr = new XMLHttpRequest();
  xhr.onload = function () {
    if (xhr.status !== 200)
      callback("[" + xhr.status + "]" + xhr.responseText, null);
    else callback(null, JSON.parse(xhr.responseText));
  };
  xhr.open(method, url, true);
  //xhr.setRequestHeader("");
  if (!data) xhr.send();
  else {
    xhr.setRequestHeader("Content-Type", "text/plain; charset=UTF-8");
    xhr.send(data);
  }
}

function sendpreflight(ping, callback) {
  const xhr = new XMLHttpRequest();
  xhr.onload = function () {
    if (xhr.status !== 200)
      callback("[" + xhr.status + "]" + xhr.responseText, null);
    else callback(null, JSON.parse(xhr.responseText));
  };
  xhr.open("POST", "http://localhost:"+ping+"/ping");
  xhr.setRequestHeader("X-PINGOTHER", "pingpong");
  xhr.setRequestHeader("Content-Type", "text/xml");
  xhr.send("<person><name>Me</name></person>");
}

window.onload = function(){
  document.getElementById("form").addEventListener("submit", function(e){
    e.preventDefault();
    const text = document.getElementById("input-text").value;
    console.log(text);
    send("POST", "/item", text, function(err, res){
      if(err){
        document.getElementById("output").innerHTML = res;
        console.log(err);
      } else {
        let str = "";
        res.entries.forEach(element => {
          str += element + " "
        });
        document.getElementById("output").innerHTML = str;
      }
    });
  });
  document.getElementById("form2").addEventListener("submit", function(e){
    e.preventDefault();
    const text = document.getElementById("input-text2").value;
    send("DELETE", "/item", text, function(err, res){
      if(err){
        document.getElementById("output").innerHTML = res;
        console.log(err);
      } else {
        let str = "";
        res.entries.forEach(element => {
          str += element + " "
        });
        document.getElementById("output").innerHTML = str;
      }
    });
  });
  document.getElementById("form3").addEventListener("submit", function(e){
    e.preventDefault();
    const text = document.getElementById("input-text-original").value;
    const repl = document.getElementById("input-text-replace").value;
    send("PATCH", "/item", text+","+repl, function(err, res){
      if(err){
        document.getElementById("output").innerHTML = res;
        console.log(err);
      } else {
        let str = "";
        res.entries.forEach(element => {
          str += element + " "
        });
        document.getElementById("output").innerHTML = str;
      }
    });
  });
  document.getElementById("ping").addEventListener("click", function(e){
    const port = document.getElementById("port").value;
    sendpreflight(port, function(err, res){
      if(err){
        console.log(err);
      } else {
        console.log(res);
      }
    });
  });
};