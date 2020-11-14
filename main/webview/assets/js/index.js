const error_msg = document.getElementById("error_msg");
const username = document.getElementById("username");
const password = document.getElementById("password");
const form = document.getElementById("form");

const credentials_route = "/form"
const configuration_route = "/configuration"

function login_callback(){
    console.log(`ResponseType: ${this.responseType}. Response : ${this.responseText}. Http code: ${this.status}`);
    if (this.status == "403") {
        error_msg.innerText = "Username or password is incorrect";
    } else if (this.status == "200") {
        window.location.href = configuration_route;
    }
} 

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
        response.addEventListener("load", login_callback);
        response.open("post",credentials_route); // false to sync execution
        
        const json = { username: username.value, password: password.value };
        
        response.responseType = "text";
        response.send(JSON.stringify(json));
    }
});


