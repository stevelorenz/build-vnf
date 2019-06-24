Network Function Virtualization
===============================

The introduction of NFV can be found on `Wikipedia <https://en.wikipedia.org/wiki/Network_function_virtualization>`_.
There is also a good introduction/motivation of NFV on ETSI's `NFV technology page <https://www.etsi.org/technologies/nfv/nfv>`_.

    In the same way that applications are supported by dynamically configurable and fully automated cloud environments,
    virtualized network functions allow networks to be **agile** and capable to respond **automatically** to the needs
    of the traffic and services running over it.

Th focus of Build-VNF project is on build, management and orchestration of virtualized network functions. For other
components of ETSI's NFV architecture, including VIM, NFVI, NFVO, VNFM, STOA lightweight frameworks/tools are chosen to
build the testbed. So the targets of Build-VNF are VNF **implementation** and **management**.

Following are some useful informations that I mainly read from ETSI NFV technology page, `NFVWiki
<https://nfvwiki.etsi.org/index.php?title=Main_Page>`_ and other blogs:

#. ETSI is running an innovative NFV plugtest program.The NFV Plugtests Programme is supported by a number of key open
   source communities working in NFV solutions, such as ETSI OSM, Open Baton, Open Stack, Open Air Interface, Sonata,
   ONAP, OpenDayLight, Fog05 and OPNFV.

#. OpenStack is the most used NFVI (NFV Infrastructure) platform. So, I don't like OpenStack at all... But performance
   benchmarks may need it...

#. The Intel has done some work to accelerate NFV on Kubernetes. This is a short presentation on Intel's brightTALK
   platform (name: Kubernetes and Network Function Virtualization (NFV): Friends or Foes?). It focus on bare mental
   setup using Kubernetes as the MANO platform. Solutions to enable NFV on Kubernetes are listed in the presentation.
   virtio_user interface is used in containerized VNF application.

#. NFV acceleration use cases: IPSec tunneling, Trans-coding, DPI; Load balncing and NAT, packet flow routing.

#. NFV acceleration interfaces requirements: state transfer, crypto operation such as IPSec/SSL/TLS/HTTP2, random
   number/key management. Support acceleration of video/audio operations, e.g. trans-coding, codec. vIPSec cryptography
   acceleration with resource management. vCPE SFC acceleration.

#. Virtual Customer Premises Equipment(vCPE): Customer premises equipment such as routers, firewalls, VPN, and NAT that
   used to require dedicated hardware is now moving to virtual software-based functions. As the price per bit continues
   to decrease, revenues are falling in lockstep. So providers are looking to add services beyond connectivity that add
   to their revenue streams, market competitiveness, and custom loyalty. Those services include managed IP-VPNs for
   branch offices and remote employees. These kinds of services are the operators' best chance to differentiate their
   services from their competition.

#. From NFVWiki:

   #. More recent implementations are starting to introduce VNFs based on containers. The use of container-based
         virtualisation is getting more considered, due to the request for cloud-native VNFs.
   #. NFVO and VNFM are funtional blocks providing different functions. However, actual implementations can subsume into
         one single entity both functionalities.
   #. Ques: Why do we need to use SDN, if NFVO can perform its function? Ans: The NFVO is responsible for resource
         orchestration and Network service orchestration, the SDN is responsible for DC networking. When it comes to the
         networking part of services, the inter-VNF paths are defined as Network Service Descriptor and processed by
         NFVO, this information is used to derive the contents of request sent to VIM, which in turn can delegate the
         configuration of infra-routers and switches to SDN.


Useful Links
------------

* `The ETSI standard for NFV architecture framework
  <https://www.etsi.org/deliver/etsi_gs/NFV/001_099/002/01.01.01_60/gs_NFV002v010101p.pdf>`_.
* `Accelerating NFV Delivery with OpenStack
  <https://object-storage-ca-ymq-1.vexxhost.net/swift/v1/6e4619c416ff4bd19e1c087f27a43eea/www-assets-prod/telecoms-and-nfv/OpenStack-Foundation-NFV-Report.pdf>`_
* `Kubernetes and Network Function Virtualization (NFV): Friends or Foes?
  <https://www.brighttalk.com/webcast/12229/347830/kubernetes-and-network-function-virtualization-nfv-friends-or-foes>`_
* `ETSI NFV Tutorials <https://www.etsi.org/technologies/nfv/nfv-tutorials>`_
* `NFV Acceleration Technology Overview <https://www.brighttalk.com/clients/js/common/1.8.0/app.html?domain=https%3A%2F%2Fwww.brighttalk.com%2F&dataDomain=https%3A%2F%2Fwww.brighttalk.com%2F&secureDomain=https%3A%2F%2Fwww.brighttalk.com%2F&player=webcast_player_widescreen&appName=webcast_player&playerName=html&channelId=12761&communicationId=223025&width=460&height=357&autoStart=false&embedUrl=https%3A%2F%2Fwww.etsi.org%2Ftechnologies%2Fnfv%2Fnfv-tutorials&messagingWindow=https%3A%2F%2Fwww.etsi.org%2Ftechnologies%2Fnfv%2Fnfv-tutorials&categories=undefined&uniqueEmbedId=45755694&iframeId=bt-webcast-player_widescreen-4&nextWebcast=undefined&prevWebcast=undefined>`_
* `Ciena D-NFVI Software <https://www.ciena.com/products/distributed-nfvi>`_
