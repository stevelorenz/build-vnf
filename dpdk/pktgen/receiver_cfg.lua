#! /usr/bin/env lua
--
-- About: Lua config for receiver

package.path = package.path ..";?.lua;test/?.lua;app/?.lua;"

printf("Lua Version      : %s\n", pktgen.info.Lua_Version);
printf("Pktgen Version   : %s\n", pktgen.info.Pktgen_Version);
printf("Pktgen Copyright : %s\n", pktgen.info.Pktgen_Copyright);
printf("Pktgen Authors   : %s\n", pktgen.info.Pktgen_Authors);

--prints("pktStats", pktgen.pktStats("all"));

--pktgen.screen("off");
pktgen.set("0", "count", 100);
pktgen.set("0", "proto", "udp");
pktgen.set("0", "type", "ipv4");
pktgen.set_ipaddr("0", "src","192.168.0.1/24");
