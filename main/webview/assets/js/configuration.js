const enviar = document.getElementById("enviar");
const dropbox = document.getElementById("dropbox");

// on event button click
enviar.addEventListener( "click" ,(e) => {
    e.preventDefault();
    
    const file = document.getElementById("firmware").files[0];

    if(file == undefined){
        console.log("Not a file");
    }
    else{
        console.log("click");
        console.log(file);
    }    
});

// on event file drop in window
dropbox.addEventListener( "drop", (e) => {
    e.stopPropagation();
    e.preventDefault();
    
    const data = e.dataTransfer;
    const file = data.files[0];

    if(file == undefined){
        console.log("Not a file");
    }
    else if(file.name.search(".bin") === -1){ // verify if file is a .bin file
        console.log("Not a .bin file")
    }
    else{
        console.log("dropped");
        console.log(file);
    }
});


/* ------------------------------------------------------------------------- */

// extras, to prevent default action os drag
dropbox.addEventListener( "dragenter", (e) => {
    e.stopPropagation();
    e.preventDefault();
});

dropbox.addEventListener( "dragover", (e) => {
    e.stopPropagation();
    e.preventDefault();
});
