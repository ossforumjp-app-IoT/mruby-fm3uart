# mruby-fm3uart
FM3 UART class

## Methods

|method|parameter(s)|return|
|---|---|---|
|UART#new|MFS ch, pin location, bit rate|(nil)|
|UART#write|8bit integer data/string/array(include 8bit integer data or string)|(nil)|
|UART#received?|(none)|TRUE/FALSE|
|UART#read|(none)|8bit integer data|
|UART#flush|(none)|(nil)|
|UART#close|(none)|(nil)|

## Constants

|constant||
|---|---|
|UART::MFS0|use MFS ch0|
|UART::MFS1|use MFS ch1|
|UART::MFS2|use MFS ch2|
|UART::MFS3|use MFS ch3|
|UART::MFS4|use MFS ch4|
|UART::MFS5|use MFS ch5|
|UART::MFS6|use MFS ch6|
|UART::MFS7|use MFS ch7|
|UART::PIN_LOC0|use pin location 0|
|UART::PIN_LOC1|use pin location 1|
|UART::PIN_LOC2|use pin location 2|

## Sample

    uart = UART.open(UART::MFS3, UART::PIN_LOC2, 115200)
    uart.write(0x85)
    uart.write("mruby")
    array = [0x21, 0x16, 0x32, "mruby", 0x1B]
    uart.write(array)
    
    if uart.received?
      string = "*receive:"
      loop do
      data = uart.read
      if data == 0x0D
        break
      else
        string << data.chr
      end
    end
    string << "*"
    uart.write(string)
    uart.flush

## License
個々のファイルについて、個別にライセンス・著作権表記があるものはそのライセンスに従います。
ライセンス表記がないものに関しては以下のライセンスを適用します。

Copyright 2015 Japan OSS Promotion Forum

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.