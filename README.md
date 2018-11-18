## Memory map

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/0e3a0af965e04767ab8cacd08d2e3569)](https://app.codacy.com/app/onkwon/yaboot?utm_source=github.com&utm_medium=referral&utm_content=onkwon/yaboot&utm_campaign=Badge_Grade_Dashboard)

	            /============\
	sector 0    | Bootloader |
	       .    |    --------|
	       .    |   | AES &  |
	       .    |   | RSA key|  Data
	       .    |------------|  /
	       m    |  BootOpt   | /
	sector m+1  |------------|/
	       .    |   OS(APP)  |
	       .    |  ----------|
	       .    |  | MAGIC1:3|
	       .    |  | LEN     |            --------
	       .    |  | IV      |  / 0x0000 | MAGIC1 | 0xDEADC0DE
	       n    |  | Hash*   | /         |--------|
	sector n+1  |------------|/   0x0004 | MAGIC2 | 0xDEADC1DE
	       .    |            |           |--------|
	       .    | New image  |    0x0008 | MAGIC3 | 0xDEADC2DE
	       .    |            |           |--------|
	            |------------|\   0x000C | Length | of E(Data)
		                   \         |--------|
				    \ 0x0010 |   IV   | AES Initial Vector
				             |--------|
				      0x0020 |  Hash* | RSApriv(HASH(E(Data)))
				             |--------|
				      0x0040 | E(Data)|
				              --------
	* Hash = RSApriv(HASH(E(Data)))

## BootOpt (1 sector)

	0x0000 | ADDR
	0x0004 | LEN
	0x0008 | HASH (64 bytes)
	0x0028 | IV (16 bytes)

* HASH is authenticated by RSA private key. So decrypt it first using the public key before comparing

## How it works

```
A1. If ADDR[0:2] matches to MAGIC1:3, go to B1
  A1-1. Before load ADDR[0:2] check flash boundary of ADDR
A2. Else compare HASH to check if APP is modified (optional)
  A2-1. If no ADDR|HASH|LEN or not match, retrieve it again from APP
  A2-2. APP must be verified after retrieving again
  A2-3. If ADDR is not the same to APP address, go with APP address
A3. Run the APP

B1. Go to C8

C1. Download new image into flash, at the sector n+1
C2. Check hash
C3. If not matches, go to C1
C4. Verify the flashed new image
  C4-1. If fail, go to C1
C5. Erase BootOpt
  C5-1. If reset occurs here, A2-1 will take place meaning you need to start again from C1
C6. Update ADDR to the new image start address, LEN, and HASH
  C6-1. Verify all the meta data above after flashing
C7. Reboot(optional)
  C7-1. A1 and B1 will take place
C8. Verify BootOpt comparing to new image meta data
  C8-1. If fail, go to C4
C9. Erase APP
C10. Write new image to APP
  C10-1. Verify
C11. Erase BootOpt
  C11-1. same to C5-1
C12. Upate ADDR, LEN, and HASH
  C12-1. Verify
C13. Reboot
```

## TODO

* Add a functionality to update booloader itself
* AES key exchange
* Diffie hellman key exchange

## NOTE

* Don't forget to lock flash from reading, using SOC protection mechanism
* Make sure the written new image at temporal flash slot is valid one before updating BootOpt. Otherwise you will get stuck booting
