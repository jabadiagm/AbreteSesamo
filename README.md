# Ábrete Sésamo
Detector de proximidad / apertura de puertas por bluetooth

![Centralita](/Centralita.jpg)

Ábrete Sésamo es un detector de proximidad basado en bluetooth que permite abrir la puerta de un hogar o de un garaje al detectar un dispositivo bluetooth dentro de un rango de 10m, aproximadamente. La centralita está formada por un arduino mega y una placa auxiliar. La placa contiene un relé para conectar al circuito de apertura y tres chips de bluetooth (2xHC05 + BLE112). El primer HC05 sirve para detectar teléfonos anteriores a los smartphones. El segundo HC05 detecta smartphones con bluetooth 1-3. Y el BLE112 sirve para smartphones con bluetooth 4. Los smartphones deben incluir una aplicación que se comunique con la centralita.