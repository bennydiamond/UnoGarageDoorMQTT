# UnoGarageDoorMQTT

Requires Arduino Ethernet Shield

Use non-latching digital Hall effect sensors for both Open and Close door state

Use MQTT to convey status, availability(via LWT) and to receive commands.

Has a retry mechanism, with max retry limit if door doesn't get to the desired state. Is only enforced when receiving a command. External door activation does not trigger this.

Prints debug log on port 80. Does not automatically refresh but is simple enough to monitor with a script and continually calling wget.