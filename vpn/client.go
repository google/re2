package main

import (
	"crypto/aes"
	"crypto/cipher"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net"
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
	data, err := os.ReadFile(filename)
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

func connectToServer(config *Config) (net.Conn, error) {
	address := fmt.Sprintf("%s:%d", config.Server.Address, config.Server.Port)
	conn, err := net.Dial("tcp", address)
	if err != nil {
		return nil, err
	}
	return conn, nil
}

func encryptAndSend(conn net.Conn, block cipher.Block, data []byte) error {
	iv := make([]byte, aes.BlockSize)
	if _, err := io.ReadFull(conn, iv); err != nil {
		return err
	}

	stream := cipher.NewCFBEncrypter(block, iv)
	writer := &cipher.StreamWriter{S: stream, W: conn}

	_, err := writer.Write(data)
	return err
}

func main() {
	config, err := loadConfig("config.json")
	if err != nil {
		log.Fatalf("Failed to load config: %v", err)
	}

	conn, err := connectToServer(config)
	if err != nil {
		log.Fatalf("Failed to connect to server: %v", err)
	}
	defer conn.Close()

	block, err := aes.NewCipher([]byte(config.Server.Encryption))
	if err != nil {
		log.Fatalf("Failed to create cipher: %v", err)
	}

	data := []byte("Hello, VPN server!")
	err = encryptAndSend(conn, block, data)
	if err != nil {
		log.Fatalf("Failed to send data: %v", err)
	}

	fmt.Println("Data sent successfully")
}
