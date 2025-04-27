package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

type Config struct {
	Server struct {
		Address    string `json:"address"`
		Port       int    `json:"port"`
		Encryption string `json:"encryption"`
	} `json:"server"`
	Client struct {
		Username string `json:"username"`
		Password string `json:"password"`
	} `json:"client"`
}

func loadConfig(filename string) (*Config, error) {
	data, err := ioutil.ReadFile(filename)
	if err != nil {
		return nil, err
	}
	var config Config
	err = json.Unmarshal(data, &config)
	if err != nil {
		return nil, err
	}
	return &config, nil
}

func main() {
	config, err := loadConfig("config.json")
	if err != nil {
		log.Fatalf("Failed to load config: %v", err)
	}

	fmt.Printf("Server Address: %s\n", config.Server.Address)
	fmt.Printf("Server Port: %d\n", config.Server.Port)
	fmt.Printf("Encryption: %s\n", config.Server.Encryption)
	fmt.Printf("Client Username: %s\n", config.Client.Username)
	fmt.Printf("Client Password: %s\n", config.Client.Password)

	// Initialize VPN service here using the configuration settings
}
