1. encapsulate MIB and PBCH logic into class
2. Collect info from MIB for use in PDCCH decoding
3. Decode PDCCH, get SIB1 location from 1_0
4. Decode SIB1, get serving-cell-config-common and UL-config and rach-configCommon
5. Package all collected information into unique measurements, based on the message
6. send data to influxdb and prach agent
