# BART-watcher-ESP8266
Low power device to display relevant BART departures, ESP8266 and SSD1306 OLED display

## Overview
I frequently commute on BART and wanted a convenient way to tell when I should start walking to 
the train station, ideally something that is always accesible. Using a ESP8266 board and a ~1 inch
screen, I was able to see departing trains that meet my schedule.

The top line shows the date / time the station information was collected, and the departing 
station. The following lines are trains that are usable for the destination station, with the 
preferred (direct) lines in large type, and less-preferred (transfer needed) in small type. 
Finally, if a train is within the sweet spot where it isn't departing too soon to make it to 
the station but you also won't be waiting at the station too long, the departure time is 
highlighted.

* A number is minutes until departure
* "L" means the train is currently leaving
* "X" means the trains was cancelled (somewhat frequent these days)

In the example below, these are trains leaving Embarcadero for the North Berkeley station (not
shown), Antioch and Pittsburg/Bay Point lines require a transfer, Richmond line is direct.

![Display close-up](https://brett.durrett.net/wp-content/uploads/2022/11/BART-watcher-screen-closeup.jpeg)

## Required Hardware
* ESP8266 board, any should work, [here's some](https://www.amazon.com/gp/product/B07RNX3W9J/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1)
* SSD1306 OLED display, other types may work, I used [this one](https://www.amazon.com/gp/product/B01IWGXUAK/ref=ppx_yo_dt_b_asin_title_o01_s00?ie=UTF8&psc=1)

The completed build needs 4 wires and looks a little something like this:
![BART watcher full build](https://brett.durrett.net/wp-content/uploads/2022/11/BART-watcher-full-build-scaled.jpeg)

## Thank You
* [ThingPulse OLED SSD1306 (ESP8266/ESP32/Mbed-OS)](https://github.com/ThingPulse/esp8266-oled-ssd1306) rendering library
* [BART Legacy API](https://www.bart.gov/schedules/developers/api) for being early pioneers to the API game

## To Do
- [ ] Configuration requires a compile, this should be user configuable, likely via web page
- [ ] Consider adding destination station to the display to reduce errors
- [ ] 3D Print some form of a case to make it tidy on a desk
