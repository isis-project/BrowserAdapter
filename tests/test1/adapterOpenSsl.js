 //Regression TestCases
		
		
		function testRegression(){
			
				
			 var rand= Math.floor(Math.random()*12);    
			i++;     
	if( $('ran').checked)
		var x= rand;
		else
		x=i;			                
   switch (x){
case 1:
adapter.openURL("https://www.verisign.com/");
break;
case 2:
adapter.openURL("https://www.aa.com/");
break;
case 3:
adapter.openURL("https://www.bankofamerica.com/");
break;
case 4:
adapter.openURL("https://www.sdccu.com/");

break;
case 5:
adapter.openURL("https://www.accountonline.com/"); 
break;
case 6:
adapter.openURL("https://us.etrade.com/e/t/home/");
break;
case 7:		
        adapter.openURL( "https://www.gmail.com/" );         
		break;
case 8:		
        adapter.openURL("https://www.fidelity.com/");
        break;
case 9:		
        adapter.openURL("https://www.fortify.net/sslcheck.html/");        
		break;
		
case 10:		
        adapter.openURL("https://www.google.com/accounts/ManageAccount/");        
		break;
		
case 11:		
        adapter.openURL("https://www.paypal.com/");        
		break;

default : 
adapter.openURL("https://login.yahoo.com//");
console.log("end of loop " + j);
}           
                   if(i==12){
							i=0;
							j++;
							}
                        var t=setTimeout(testRegression,2000);
						        
                   //  number of loops
						if (j == $('loop').value) {
							clearTimeout(t);
							return;
						}    
			
		}
		    var i=0;
			var j=0;
		testRegression();
		
	
	