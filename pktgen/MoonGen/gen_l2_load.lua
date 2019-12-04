#! /usr/bin/env lua
--
-- gen_l2_load.lua

local mg     = require "moongen"
local memory = require "memory"
local device = require "device"
local ts     = require "timestamping"
local stats  = require "stats"
local hist   = require "histogram"

local PKT_SIZE  = 1400
local ETH_DST   = "11:12:13:14:15:16"

local function getRstFile(...)
    local args = { ... }
    for i, v in ipairs(args) do
        result, count = string.gsub(v, "%-%-result%=", "")
        if (count == 1) then
            return i, result
        end
    end
    return nil, nil
end

function configure(parser)
    parser:description("Gen")
    parser:argument("dev", "Device to transmit/receive from."):convert(tonumber)
    parser:option("-r --rate", "Transmit rate in Mbit/s."):default(1):convert(tonumber)
end

function master(args)
    local dev = device.config({port = args.dev, rxQueues = 1, txQueues = 1})
    device.waitForLinks()
    dev:getTxQueue(0):setRate(args.rate)
    mg.startTask("loadSlave", dev:getTxQueue(0))
    stats.startStatsTask{dev}
    mg.waitForTasks()
end

function loadSlave(queue)
    local mem = memory.createMemPool(function(buf)
        buf:getEthernetPacket():fill{
            ethSrc = txDev,
            ethDst = ETH_DST,
            ethType = 0x1234
        }
    end)
    local bufs = mem:bufArray()
    while mg.running() do
        bufs:alloc(PKT_SIZE)
        queue:send(bufs)
    end
end
