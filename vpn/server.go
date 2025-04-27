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

func handleConnection(conn net.Conn, block cipher.Block) {
	defer conn.Close()

	iv := make([]byte, aes.BlockSize)
	if _, err := io.ReadFull(conn, iv); err != nil {
		log.Printf("Failed to read IV: %v", err)
		return
	}

	stream := cipher.NewCFBDecrypter(block, iv)
	reader := &cipher.StreamReader{S: stream, R: conn}

	buf := make([]byte, 1024)
	for {
		n, err := reader.Read(buf)
		if err != nil {
			if err != io.EOF {
				log.Printf("Failed to read data: %v", err)
			}
			break
		}
		fmt.Printf("Received: %s\n", buf[:n])
	}
}

func startServer(config *Config) {
	address := fmt.Sprintf("%s:%d", config.Server.Address, config.Server.Port)
	listener, err := net.Listen("tcp", address)
	if err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
	defer listener.Close()

	block, err := aes.NewCipher([]byte(config.Server.Encryption))
	if err != nil {
		log.Fatalf("Failed to create cipher: %v", err)
	}

	log.Printf("Server started on %s", address)
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Printf("Failed to accept connection: %v", err)
			continue
		}
		go handleConnection(conn, block)
	}
}

func main() {
	config, err := loadConfig("config.json")
	if err != nil {
		log.Fatalf("Failed to load config: %v", err)
	}

	startServer(config)
}
