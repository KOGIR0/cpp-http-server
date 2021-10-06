document.addEventListener("DOMContentLoaded", ready);

function ready()
{
    document.getElementsByTagName("body")[0].style['background-color'] = "black";
    document.getElementsByTagName("body")[0].style['color'] = "white";

    let user = {
        name: 'John',
        surname: 'Smith'
    };
    
    let response = fetch('/article/fetch/post/user', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json;charset=utf-8'
        },
        body: JSON.stringify(user)
    })
    .then(res => res.json())

    console.log(response);
}