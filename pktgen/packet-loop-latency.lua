--- A simple UDP packet generator
local mg     = require "moongen"
local device = require "device"
local stats  = require "stats"
local log    = require "log"
local memory = require "memory"
local arp    = require "proto.arp"
local dpdkc  = require "dpdkc"
local ts     = require "timestamping"
local timer  = require "timer"
local hist   = require "histogram"

local TSYNCRXCTL = 0x0000B620

-- set addresses here
local DST_MAC       = "d0:e1:40:8d:a8:00" -- resolved via ARP on GW_IP or DST_IP, can be overriden with a string here
local PKT_LEN       = 100
local SRC_IP        = "192.168.2.104"
local DST_IP        = "192.168.2.102"
local SRC_PORT_BASE = 319 -- actual port will be SRC_PORT_BASE * random(NUM_FLOWS)
local DST_PORT      = 319
local NUM_FLOWS     = 1000
-- used as source IP to resolve GW_IP to DST_MAC
-- also respond to ARP queries on this IP
local ARP_IP	= SRC_IP
-- used to resolve DST_MAC
local GW_IP		= DST_IP


-- the configure function is called on startup with a pre-initialized command line parser
function configure(parser)
	parser:description("Edit the source to modify constants like IPs and ports.")
	parser:argument("dev", "Devices to use."):args(1):convert(tonumber)
	parser:option("-t --threads", "Number of threads per device."):args(1):convert(tonumber):default(1)
	parser:option("-r --rate", "Transmit rate in Mbit/s per device."):args(1)
	parser:option("-o --output", "File to output statistics to")
	parser:option("-s --seconds", "Stop after n seconds")
	parser:flag("-a --arp", "Use ARP.")
	parser:flag("--csv", "Output in CSV format")
	parser:flag("--ts", "Use hardware timestamping")
	return parser:parse()
end

function master(args)
	log:info("Check out MoonGen (built on lm) if you are looking for a fully featured packet generator")
	log:info("https://github.com/emmericp/MoonGen")

	-- configure devices and queues
	local arpQueues = {}
	-- arp needs extra queues
	local dev = device.config{port = args.dev, txQueues = 2, rxQueues = 2}
	if args.arp then
		table.insert(arpQueues, { rxQueue = dev:getRxQueue(1), txQueue = dev:getTxQueue(args.threads), ips = ARP_IP })
	end
	device.waitForLinks()

	-- start ARP task and do ARP lookup (if not hardcoded above)
	if args.arp then
		arp.startArpTask(arpQueues)
		if not DST_MAC then
			log:info("Performing ARP lookup on %s, timeout 3 seconds.", GW_IP)
			DST_MAC = arp.blockingLookup(GW_IP, 3)
			if not DST_MAC then
				log:info("ARP lookup failed, using default destination mac address")
				DST_MAC = "01:23:45:67:89:ab"
			end
		end
		log:info("Destination mac: %s", DST_MAC)
	end

	
	local txQueue = dev:getTxQueue(0)
	local rxQueue = dev:getRxQueue(0)
	if args.rate then
		txQueue:setRate(args.rate / args.threads)
	end

	mg.startTask("latencyTest", txQueue, rxQueue, DST_MAC, args.ts)

	if args.seconds then
		mg.setRuntime(tonumber(args.seconds))
	end

	mg.waitForTasks()
end

function latencyTest(txQueue, rxQueue, dstMac, useTimestamp)
	-- memory pool with default values for all packets, this is our archetype
	local mempool = memory.createMemPool(function(buf)
		buf:getUdpPtpPacket():fill{
			-- fields not explicitly set here are initialized to reasonable defaults
			ethSrc = queue, -- MAC of the tx device
			ethDst = dstMac,
			ip4Src = SRC_IP,
			ip4Dst = DST_IP,
			udpSrc = SRC_PORT,
			udpDst = DST_PORT,
			pktLength = PKT_LEN
		}
	end)
	-- a bufArray is just a list of buffers from a mempool that is processed as a single batch
	local bufs = mempool:bufArray(1)
	local rxBufs = memory.bufArray()

	local seq = 1
	if useTimestamp then
		txQueue:enableTimestamps()
		rxQueue:enableTimestamps()
	end

	while mg.running() do -- check if Ctrl+c was pressed
		local numPkts = 0
		-- this actually allocates some buffers from the mempool the array is associated with
		-- this has to be repeated for each send because sending is asynchronous, we cannot reuse the old buffers here
		bufs:alloc(PKT_LEN)
		if useTimestamp then
			bufs[1]:enableTimestamps()
		end
		local pkt = bufs[1]:getUdpPtpPacket()
		local expectedSeq = seq
		pkt.udp:setSrcPort(SRC_PORT_BASE)
		pkt.ptp:setSequenceID(expectedSeq)
		seq = (seq + 1) % 2^16
		bufs:offloadUdpChecksums()
		-- send out all packets and frees old bufs that have been sent
		local t1 = mg.getTime() --t1
		txQueue:send(bufs)
		if useTimestamp then
			local t2 = txQueue:getTimestamp(500) --t2
			if t2 then
				local timer = timer:new(0.001)
				while timer:running() do
					local rx = rxQueue:tryRecv(rxBufs, 100)
					t4 = mg.getTime() --t4
					numPkts = numPkts + rx
					local timestampedPkt = rxQueue.dev:hasRxTimestamp()
	--				print("timestampedPkt", timestampedPkt)
					if not timestampedPkt then
						-- NIC didn't save a timestamp yet, just throw away the packets
						rxBufs:freeAll()
					else
						for i = 1, rx do
							local buf = rxBufs[i]
							local seq = buf:getUdpPtpPacket().ptp:getSequenceID()
							--	print("seq", seq)
							if seq == expectedSeq then
								t3 = rxQueue:getTimestamp() --t3
								print("t3 - t2", (t3-t2)/10^6, "ms")
								print("t4-t3+t2-t1", (t4-t1)*10^3-(t3-t2)/10^6, "ms")
								print("---------------------")
							end
						end -- end of for	
						rxBufs:freeAll()
					end
				end -- end of while
			end -- end of if t2
		else
			local timer = timer:new(0.001)
			while timer:running() do
				local rx = rxQueue:tryRecv(rxBufs, 100)
				t4 = mg.getTime() --t4
				numPkts = numPkts + rx
				for i = 1, rx do
					local buf = rxBufs[i]
					local seq = buf:getUdpPtpPacket().ptp:getSequenceID()
					--	print("seq", seq)
					if seq == expectedSeq then
						print("t4-t1", (t4-t1)*10^3, "ms")
						print("---------------------")
					end
				end -- end of for	
				rxBufs:freeAll()
			end -- end of while
		end
	end -- end of mg.running

end



