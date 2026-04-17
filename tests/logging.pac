// Test PAC file exercising alert() and console.log() debug helpers.
function FindProxyForURL(url, host) {
  alert("checking", host);
  console.log("url:", url);
  console.log("single arg");
  return "DIRECT";
}
