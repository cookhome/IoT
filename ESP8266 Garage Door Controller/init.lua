wifi.setmode(wifi.STATION)
wifi.sta.config("SSID","password") -- change this for wifi connection
wifi.sta.sethostname("Node-Garage")
gpio.write(4, gpio.HIGH) -- this switch off door control relay
gpio.write(3, gpio.HIGH) -- this switch of light relay
gpio.mode(4, gpio.OUTPUT)
gpio.mode(3, gpio.OUTPUT)
tdoor = tmr.create()
tdoor:register(500, tmr.ALARM_SEMI, function() gpio.write(4, gpio.HIGH) end) -- door relay auto switch off in 500ms
tlight = tmr.create()
tlight:register(180000, tmr.ALARM_SEMI, function() gpio.write(3, gpio.HIGH) end) -- light relay auto switch off in 180000ms
passfind = nil
light = 0
srv=net.createServer(net.TCP)
srv:listen(80,function(conn)
    conn:on("receive", function(client,request)
        local pwd = nil;
        local pwd1 = nil;
        local pwd2 = nil;
        local buf = "HTTP/1.0 200 OK\r\nContent-Type:text/html\r\nPragma:no-cache\r\n\r\n";
        buf = buf.."<html lang=\"zh\"><body><center><p style=\"font-size:6em\"><a href=\"?LIGHT=ON\"><button style=\"background-color:red;font-size:1em\">ON</button></a>";
        buf = buf.."   <a href=\"?LIGHT=OFF\"><button style=\"background-color:green;font-size:1em\">OFF</button></a></p><br><form method=\"POST\">";
        buf = buf.."<input type=\"password\" style=\"font-size:6em\" size=\"12\" name=\"PASSWORD\"><br><br><br><input type=\"submit\" style=\"background-color:blue;font-size:8em\" value=\"DOOR\"></form></center></body></html>";
        client:send(buf);
        if(passfind == nil)then -- no pending POST password
            passfind = string.find(request, "POST /(.*) HTTP");
            if(passfind ~= nil)then
                -- post data processing
                pwd1 = string.find(request, "PASSWORD=", passfind);
                if(pwd1 ~= nil)then
                    passfind = nil; -- found password, make no more pending POST password
                    pwd = string.sub(request, pwd1+9, #request); -- obtain password
                end
            else
                pwd1 = string.find(request, "GET /%?LIGHT=(.+) HTTP");
                if(pwd1 ~= nil)then
                    -- found GET light control data
                    pwd2 = string.find(request, " HTTP", pwd1);
                    pwd = string.sub(request, pwd1+12, pwd2-1); -- get light control command
                    if(pwd == "ON")then
                        gpio.write(3, gpio.LOW); -- switch light on
                        light = 1; -- mark light is switched on manually
                        tlight:stop(); -- stop possible auto light off
                    elseif(pwd == "OFF")then
                        gpio.write(3, gpio.HIGH); -- switch light on
                        light = 0;
                        tlight:stop(); -- stop possible auto light off
                    end
                    pwd=nil; no further processing of data
                end
            end            
        else
            -- pending POST password processing (for IOS devices)
            passfind = nil; // nor more pending POST password
            pwd1 = string.find(request, "PASSWORD=");
            if(pwd1 ~= nil)then pwd = string.sub(request, pwd1+9, #request) end -- obtain password if there is
        end
        if(pwd == "12345678")then -- match password
            -- switch on door control relay for 500ms
            gpio.write(4, gpio.LOW);
            tdoor:start();
            if(light == 0)then -- light is not switched manually
                -- if light is not switched manually, switch it on for predefined time
                gpio.write(3, gpio.LOW);
                tlight:start();
            end                      
        end
        if(passfind == nil)then client:close() end -- no more pending POST password
        collectgarbage();
    end)
end)
