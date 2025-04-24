# Weather Station
Sääasemaprojektin arduinokoodi

Tällä hetkellä analogisen ja digitaalisen signaalin luku toiminnassa.
Näppäimistöllä voi valita mitä haluaa näytettävän LCD- näytöllä:<br>
1: Näyttää onko saatu DHCP:llä IP- osoite ja onko yhteys MQTT- brokeriin.<br>
2: Näyttää signaalien suurimmat ja pienimmät arvot.<br>
3: Näyttää signaalien arvot painallushetkellä.<br>
A: Ei tee tällä hetkellä mitään.

Säädata lähetetään MQTTllä tietokantaan, ja datan näkee alla olevista linkeistä

Valoisuus: http://webapi19sa-1.course.tamk.cloud/v1/weather/w727_valoisuus

Ilmankosteus: http://webapi19sa-1.course.tamk.cloud/v1/weather/w727_kosteusIn

