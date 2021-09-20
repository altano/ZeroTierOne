/*
 * Copyright (c)2019 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2023-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

package cli

import (
	"fmt"
	"os"

	"zerotier/pkg/zerotier"
)

// Status shows service status info
func Status(basePath, authToken string, args []string, jsonOutput bool) {
	var status zerotier.APIStatus
	apiGet(basePath, authToken, "/status", &status)

	if jsonOutput {
		fmt.Println(jsonDump(&status))
	} else {
		online := "ONLINE"
		if !status.Online {
			online = "OFFLINE"
		}
		fmt.Printf("%.10x: %s %s\n", uint64(status.Address), online, status.Version)
		fmt.Printf("\tidentity:\t%s\n", status.Identity.String())
		fmt.Printf("\tports:\t%d %d %d\n", status.Config.Settings.PrimaryPort, status.Config.Settings.SecondaryPort, status.Config.Settings.TertiaryPort)
		fmt.Printf("\tport search:\t%s\n", enabledDisabled(status.Config.Settings.PortSearch))
		fmt.Printf("\tport mapping (uPnP/NAT-PMP):\t%s\n", enabledDisabled(status.Config.Settings.PortMapping))
		fmt.Printf("\tmultipath mode:\t%d\n", status.Config.Settings.MuiltipathMode)
		fmt.Printf("\tblacklisted interface prefixes:\t")
		for i, bl := range status.Config.Settings.InterfacePrefixBlacklist {
			if i > 0 {
				fmt.Print(" ")
			}
			fmt.Print(bl)
		}
		fmt.Printf("\n\texplicit external addresses: ")
		for i, ea := range status.Config.Settings.ExplicitAddresses {
			if i > 0 {
				fmt.Print(" ")
			}
			fmt.Print(ea.String())
		}
		fmt.Printf("\n\tsystem interface addresses: ")
		for i, a := range status.InterfaceAddresses {
			if i > 0 {
				fmt.Print(" ")
			}
			fmt.Print(a.String())
		}
		fmt.Printf("\n\tmapped external addresses: ")
		for i, a := range status.MappedExternalAddresses {
			if i > 0 {
				fmt.Print(" ")
			}
			fmt.Print(a.String())
		}
		fmt.Printf("\n")
	}

	os.Exit(0)
}