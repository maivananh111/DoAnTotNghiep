document.addEventListener("DOMContentLoaded", function() {
	
	
  	var gateway = `ws://${window.location.hostname}/ws`;
  	var websocket;
	
	window.addEventListener('load', onLoad);
	
	function onLoad(event) {
	    initWebSocket();
	}
	
	function onOpen(event) {
	    console.log('Connection opened');
	}
	
	function onClose(event) {
	    console.log('Connection closed');
	    setTimeout(initWebSocket, 2000);
	}
	
	function onMessage(event) {
	    console.log('Message received: ', event.data);
	}
	
	function initWebSocket() {
	    websocket = new WebSocket(gateway);
	    websocket.onopen    = onOpen;
	    websocket.onclose   = onClose;
	    websocket.onmessage = onMessage;
	}
	
    var current_fs, next_fs, previous_fs; //fieldsets

    var nextButtons = document.querySelectorAll(".next");
    var previousButtons = document.querySelectorAll(".previous");

    var username;
    var password;

    var selectedOption;
	
	var selectedRS232;
	
 	var showButtons; 

	var mbdesc;
	
    nextButtons.forEach(function(button) {
        button.addEventListener("click", function() {

            current_fs = this.parentElement;
            next_fs = this.parentElement.nextElementSibling;

            /* Check user name and password */
            var usernameInput = current_fs.querySelector('input[name="username"]');
            var passwordInput = current_fs.querySelector('input[name="pass"]');
            
            if (usernameInput && passwordInput) {
                username = usernameInput.value;
                password = passwordInput.value;
            }
            if(username == "admin" && password =="admin")
            {
                //activate next step on progressbar using the index of next_fs
                var progressbarItems = document.querySelectorAll("#progressbar li");
                var fieldsets = document.querySelectorAll(".child_form");
                var next_fs_index = Array.from(fieldsets).indexOf(next_fs);
                
                if (next_fs_index !== -1) {
                    progressbarItems[next_fs_index].classList.add("active");
                }

                //show the next fieldset
                current_fs.style.display = "none";
                next_fs.style.display = "block";
            }
        });
    });

    previousButtons.forEach(function(button) {
        button.addEventListener("click", function() {

            current_fs = this.parentElement;
            previous_fs = this.parentElement.previousElementSibling;

            //de-activate current step on progressbar
            var progressbarItems = document.querySelectorAll("#progressbar li");
            var fieldsets = document.querySelectorAll(".child_form");
            var current_fs_index = Array.from(fieldsets).indexOf(current_fs);
            
            if (current_fs_index !== -1) {
                progressbarItems[current_fs_index].classList.remove("active");
            }

            //show the previous fieldset
            current_fs.style.display = "none";
            previous_fs.style.display = "block";

        });
    });
	


    document.getElementById("application-select").addEventListener("change", function() {
        selectedOption = this.value;

        for (var i = 0; i < document.getElementsByClassName("show_setting").length; i++) {
            document.getElementsByClassName("show_setting")[i].style.display = "none";
        }
        
        switch (this.value) {
            case "rs485":
                document.getElementById("rs485").style.display = "block";
                break;
            case "rs232":
                document.getElementById("rs232").style.display = "block";
                break;
            case "4-20mA":
                document.getElementById("4_20mA").style.display = "block";
                break;
            case "0-10Vdc":
                document.getElementById("0_10Vdc").style.display = "block";
                break;
            default:
                break;
        }
     });
 	
 	document.getElementById("rs232_select").addEventListener("change", function() {
        selectedRS232 = this.value; 
  	});

	document.getElementById("login_setting_form_button").addEventListener("click", function() {
		document.getElementById('login_setting_form').style.display='block'; 
		this.style.display='none';
	});

	document.getElementById("exit_button").addEventListener("click", function() {
		document.getElementById('login_setting_form').style.display='none';
		document.getElementById('login_setting_form_button').style.display='inline-block';

	});

    const actions = {
        "deveui": (value) => {
            document.querySelector('input[name="deveui"]').value = value;
        },
        "appeui": (value) => {
            document.querySelector('input[name="joineui"]').value = value;
        },
        "appkey": (value) => {
            document.querySelector('input[name="appkey"]').value = value;
        },
        "period": (value) => {
            document.querySelector('input[name="otaaperiod"]').value = value;
        }
    };

    function handleKey(key, value) {
        if (actions[key])
            actions[key](value);
    }
      
    /* Show RS485 device html */      
    function displayRS485Devices(mbdesc) {
	    var keys = Object.keys(mbdesc);
	    
	    // Clear the existing content in the rs485 element
	    document.getElementById("rs485").innerHTML = "";
	    
	    // Iterate over each key in the mbdesc object
	    for (var i = 0; i < keys.length; i++) {
	        var name = keys[i];
	        var values = mbdesc[name];
	        
	        var uart_baud = values.baud;
	        var desc = values.desc;
	        var id = values.addr;
	        var func_code = values.fcode;
	        var reg_addr = values.reg;
	        var read_quanty = values.rqty;
	        
	        // Create HTML content for each device
	        var nameHTML = `<h2>${name}</h2>`;
	        var listRS485 = `
	            <ul>
	                <li>Uart baudrate: ${uart_baud}</li>
	                <li>Description: ${desc}</li>
	                <li>Id: ${id}</li>
	                <li>Function code: ${func_code}</li>
	                <li>Register address: ${reg_addr}</li>
	                <li>Read quanty: ${read_quanty}</li>
	            </ul>`;
	        // Append the content to the rs485 element
	        document.getElementById("rs485").innerHTML += nameHTML + listRS485;
	    }
	    
	    // Add input fields and buttons for deleting and adding devices
	    var typeDev485toDel = `
	        <input type="text" name="delete_RS485" placeholder="Type RS485 name to delete" />
	        <input type="button" name="delete_device_rs485" class="delete_device action-button" value="Delete device" />
	        <input type="button" name="add_device_rs485" class="add_device action-button" value="Add device"/>`;
	    document.getElementById("rs485").innerHTML += typeDev485toDel;
	}
    
        
    /* Check modbus device size */
    var mb_dt_size = 0;
    function onMessage(event) {
		
	    console.log(event.data);
	    var myObj = JSON.parse(event.data);       
	    
	    if(myObj["wan"] !== undefined) {
			var keys = Object.keys(myObj["wan"]);
			var wan_obj = myObj["wan"]
			for( var i = 0; i< keys.length; i++) {
	   			var name = keys[i];
			    var value = wan_obj[name];
				handleKey(name,value);
			}
		}
		
		if (myObj["mbdesc"] !== undefined) {
			
		  	mbdesc = myObj["mbdesc"];
			
			console.log(mbdesc);
			
		  	displayRS485Devices(mbdesc);
	    		
    		document.getElementById("rs485").addEventListener("click", function(event) {
	        	if (event.target && event.target.classList.contains("add_device")) {
	            	handleAddDevice();
	        	}
	        	else if (event.target && event.target.classList.contains("delete_device")) {
	            	handleSubDevice();
	        	}
    		});
						
			function handleSubDevice() {
				var delete_device = document.querySelector('input[name="delete_RS485"]');
				
				let keyToDel = delete_device.value;
				
				if(mbdesc.hasOwnProperty(keyToDel))
				{
					delete mbdesc[keyToDel];
															
					let mbdescObjectSend = {
					  "mbdesc": mbdesc
					};
					
					const dt_send = JSON.stringify(mbdescObjectSend);
					console.log(dt_send);
					/* Reset new html */
					displayRS485Devices(mbdescObjectSend["mbdesc"]);
					websocket.send(dt_send);
				}
			}
			
			function handleAddDevice() {
				var data = `<input type="text" name="name_rs485" placeholder="Name RS485" />
	    					<input type="text" name="uart_baurate" placeholder="Uart baurate" />
							<input type="text" name="description" placeholder="Description" />
							<input type="text" name="id" placeholder="ID" />
							<input type="text" name="function_code" placeholder="Function code" />
							<input type="text" name="register_address" placeholder="Register address" />
							<input type="text" name="read_quanty" placeholder="Read quanty" />
							<input type="button" name="show_device_rs485" class="show_device action-button" value="Show device" />`
				document.getElementById("rs485").innerHTML = data;
				document.getElementsByClassName("submit")[0].style.display = "inline-block";
			}
		}
			
        if(myObj.data != undefined)
        {
	        /* Nếu như khác số phần tử */
			if (mb_dt_size != Object.keys(myObj.data).length) {
				// Xóa nội dung hiện tại
				document.getElementById("dataview").innerHTML = "";
			
				// Cập nhật mb_dt_size
				mb_dt_size = Object.keys(myObj.data).length;
			
				// Tạo bảng và các phần tử của nó
				var table = "<table class='dataview_table'>" +
								"<thead>" +
									"<tr>" +
										"<th>Description</th>" +
										"<th>Data</th>" +
									"</tr>" +
								"</thead>" +
								"<tbody>";
			
				// Thêm các hàng dữ liệu vào bảng
				for (var i = 0; i < mb_dt_size; i++) {
					var description = "Description " + (i + 1); // Thay thế bằng dữ liệu thực tế nếu có
					var data = "Data " + (i + 1); // Thay thế bằng dữ liệu thực tế nếu có
			
					table += "<tr>" +
								"<td class='data_desc'>" + description + "</td>" +
								"<td class='data'>" + data + "</td>" +
							 "</tr>";
				}
			
				// Đóng thẻ tbody và table
				table += "</tbody></table>";
			
				// Thêm bảng vào phần tử với id 'dataview'
				document.getElementById("dataview").innerHTML = table;
			}
			
			var data = myObj.data;
        	var keys = Object.keys(data);
        	var values = Object.values(data);
        	
			var dataDescElements = document.querySelectorAll('.data_desc');

			var dataElements = document.querySelectorAll('.data');

			for (let i = 0; i < keys.length; i++) {
				
			    const key = keys[i].toUpperCase();
			    const value = values[i];	
				
				//console.log(key);
				//console.log(value);
			    dataDescElements[i].innerHTML = ""; 
			 	dataElements[i].innerHTML = "";
				dataDescElements[i].append(key+"   ");
   				dataElements[i].append(value/100);
			}
		}
    }
    

      
	document.getElementById("rs485").addEventListener("click", function(event) {
    	if (event.target && event.target.classList.contains("show_device")) {
			document.getElementById("rs485").innerHTML = "";
			document.getElementsByClassName("submit")[0].style.display = "none";
        	websocket.send("{\"shw_dev_rs485\":\"true\"}");	
    	}
	});  
      	    
	document.getElementById("4_20mA").addEventListener("click", function(event) {
    	if (event.target && event.target.classList.contains("show_value_4_20mA")) {
        	websocket.send("{\"shw_val_4_20mA\":\"true\"}");	
    	}
	});
    
    /* Thực hiện các nút CONFIGURE và SUBMIT để gửi dữ liệu ứng dụng hoặc lorawan từ web về esp32 */
  	var submitButtons = document.querySelectorAll(".submit");
	submitButtons.forEach(function(button) {
    	button.addEventListener("click", function() {
        if (this.value === "CONFIGURE") {
            switch (selectedOption) {
                case "rs485":   
                	var name = document.querySelector('input[name="name_rs485"]').value;
                    var uart_baurate = document.querySelector('input[name="uart_baurate"]').value;
                    var description = document.querySelector('input[name="description"]').value;
                    var id = document.querySelector('input[name="id"]').value;
                    var function_code = document.querySelector('input[name="function_code"]').value;
                    var register_address = document.querySelector('input[name="register_address"]').value;
                    var read_quanty = document.querySelector('input[name="read_quanty"]').value;
					
					var data = {
						desc : description,
						
						baud : uart_baurate,
						addr : id,
						fcode : function_code,
						reg : register_address,
						rqty : read_quanty,
					};
					data.baud = parseInt(data.baud);
					data.rqty = parseInt(data.rqty);
                	var data_total = {
                		[name] : data
                	};	
                	var m_data = {
						...mbdesc,
						...data_total
					}
					var s_data = {
						"mbdesc" : m_data
					}
                	
					const dt_send = JSON.stringify(s_data);

					console.log(dt_send);
					websocket.send(dt_send);
					alert("Add device ok ! Next and click submit and reset device....")
        		
                    break;
                case "rs232":
					var data = selectedRS232;
					console.log(data);
					var s_data = {
						"mpn" : data
					}
					var jsonData = JSON.stringify(s_data);
					console.log(jsonData);
            		websocket.send(jsonData);	
            		alert("Select sensor ok ! Next and click submit and reset device....")
                    break;
                case "4-20mA":
				case "0-10Vdc":
                	var minvalue = document.querySelector('input[name="minvalue"]').value;
                	var maxvalue = document.querySelector('input[name="maxvalue"]').value;
         
                	var data = {
                		min : minvalue,
                		max : maxvalue
                	};
                	
					data.min = parseFloat(data.min);
					data.max = parseFloat(data.max);
					
                	var s_data = {
						"calib" : data
					}
    	            var jsonData = JSON.stringify(s_data);
            
            		websocket.send(jsonData);	
            		alert("Calib ok ! Next and click submit and reset device....")
                    break;
                default:
                    break;
            }
            
        } 
        else if (this.value === "Submit lorawan") {
			console.log("submit lorawan clicked");
            var otaaperiod = document.querySelector('input[name="otaaperiod"]').value;
            var appkey = document.querySelector('input[name="appkey"]').value;
            var joineui = document.querySelector('input[name="joineui"]').value;

            var data = {
                period: otaaperiod,
                appkey: appkey,
                appeui: joineui
            };
            
            data.period = parseInt(data.period)
            
			var s_data = {
				"wan" : data
			}
        	
			const dt_send = JSON.stringify(s_data);
			console.log(dt_send);
			websocket.send(dt_send);
			alert("Send ok ! Waiting to reset ....")
        }
    	});
	});

});




















