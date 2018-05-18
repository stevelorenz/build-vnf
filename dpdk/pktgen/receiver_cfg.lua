#! /usr/bin/env lua
--
-- About: Lua config for receiver

package.path = package.path ..";?.lua;test/?.lua;app/?.lua;"

printf("Lua Version      : %s\n", pktgen.info.Lua_Version);
printf("Pktgen Version   : %s\n", pktgen.info.Pktgen_Version);
printf("Pktgen Copyright : %s\n", pktgen.info.Pktgen_Copyright);
printf("Pktgen Authors   : %s\n", pktgen.info.Pktgen_Authors);

-- Turn off result screen
pktgen.screen("off");

-- Call system call to sleep only works on Linux
function sleep(n)
    os.execute("sleep "..n);
end

--- [[
--  Print port statistics
--  ]]
i = 0
while (i < 10) do
    prints("portRates", pktgen.portStats("0", "rate")[0]);
    prints("portStats", pktgen.portStats("all", "port")[0]);
    prints("pktStats", pktgen.pktStats("all"));
    i = i + 1;
    pktgen.delay(1000);
end
