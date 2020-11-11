const error_msg = document.getElementById("error_msg");
const username = document.getElementById("username");
const password = document.getElementById("password");
const form = document.getElementById("form");

const credentials_route = "/form"

form.addEventListener("submit", (e) => {
    e.preventDefault();
    
    let msg = [];
    
    if(username.value == null || username.value === ""){
        msg.push("Please insert username.");
    }else if(password.value == null || password.value === ""){
        msg.push("Please insert password.");
    }
    
    if(msg.length > 0){
        error_msg.innerText = msg.join(", ");
    }else{
        error_msg.innerText = "";

        const response = new XMLHttpRequest();
        response.open("post",credentials_route);
        
        const json = { username: username.value, password: password.value };

        response.responseType = "json";
        response.send(JSON.stringify(json));
    }
});

