//
// About: Server for slow-path processing.
//

package main

import (
	"context"
	"fmt"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/client"
	zmq "github.com/pebbe/zmq4"
)

func main() {

	// Default CPUPeriod 100 us
	new_rsc := container.Resources{CPUQuota: -1, CPUPeriod: 100000}
	new_config := container.UpdateConfig{Resources: new_rsc}

	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	if err != nil {
		panic(err)
	}

	containers, err := cli.ContainerList(ctx, types.ContainerListOptions{})
	if err != nil {
		panic(err)
	}

	for _, container := range containers {
		fmt.Println(container.ID)
	}

	fmt.Println("Start zmq server...")
	server, _ := zmq.NewSocket(zmq.REP)
	defer server.Close()
	server.Bind("ipc:///tmp/ffpp.sock")

	for {
		_, err := server.RecvMessage(0)
		if err != nil {
			break
		}
		_, err = cli.ContainerUpdate(ctx, containers[0].ID, new_config)
		if err != nil {
			fmt.Println("Failed to update container resource.")
			break
		}

		server.SendMessage("OK")
	}
}
