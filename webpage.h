// webpage.h

#ifndef WEBPAGE_H
#define WEBPAGE_H

const char* index_html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            background-color: aqua;
            font-family: sans-serif;
        }
    
        form {
            width:50%;
            margin: auto;
        }
        
        input {
            width:100%;
            margin:auto;
            font-size:20px;
            min-height:5vh;
        }
        
        h1 {
            text-align: center;
        }

        @media screen and (max-width: 480px) {
            form {
                width:90%;
            }
            input {
                width:100%;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WIICMATH<br>WiFi Configuration</h1>
        <form action="/configure" method="POST">
            <label for="ssid">WiFi SSID:</label><br>
            <input type="text" id="ssid" name="ssid"><br><br>
            <label for="password">WiFi Password:</label><br>
            <input type="password" id="password" name="password"><br><br>
            <input type="submit" value="Submit">
        </form>
    </div>
</body>
</html>
)";

#endif