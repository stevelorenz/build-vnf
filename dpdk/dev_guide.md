# Development Guide #

## Tools ##

## Testpmd Application ##

## Traffic Generator: pktgen ##

## Warnings ##

- In order to use the CLI program, the pktgen binary and Pktgen.lua should be in the same directory.

- The master lcore (default 0) is just used by pktgen for display and timers. So it can not used for Rx\Tx
    At least 2 cores are needed and the second core can be used for traffic generation.
