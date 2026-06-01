package main

import (
	"encoding/base64"
	"encoding/binary"
	"fmt"
	"log"
	"math"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

const SERVER_IP = ""
const LOGIN_URL = "g-topia-login.vercel.app"
const MAINTENANCE_MESSAGE = "`#Maintenance"

/**
* actually theres a diffirent system under it but its ok (encryption)
* and saving just random data
 */
func encryptData(data []byte, key int) []byte {
	result := make([]byte, len(data))
	for i := range data {
		b := data[i] ^ byte(key)
		b = (b << 5) | (b >> 3)

		result[i] = b
		key += int(b)
	}

	return result
}

func fileExists(path string) bool {
	_, err := os.Stat(path)
	return err == nil
}

func loginHandler(res http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodPost {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	if err := req.ParseForm(); err != nil {
		fmt.Println("Failed to parse form")
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	if len(req.Form) == 0 {
		fmt.Println("Parsed form but it's empty")
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	versionStr := req.Form.Get("version")
	platformStr := req.Form.Get("platform")
	protoStr := req.Form.Get("protocol")

	if len(versionStr) == 0 || len(platformStr) == 0 || len(protoStr) == 0 {
		fmt.Printf("Unable to get datas version:%v platform:%v protocol:%v\n", versionStr, platformStr, protoStr)
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	version, versionErr := strconv.ParseFloat(versionStr, 32)
	if versionErr != nil || version < 0 || version > 6 {
		fmt.Printf("Failed to parse version %v\n", versionStr)
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	platform, platformErr := strconv.Atoi(platformStr)
	if platformErr != nil || platform < 0 || platform > 10 {
		fmt.Printf("Failed to parse platform %v\n", platformStr)
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	proto, protoErr := strconv.Atoi(protoStr)
	if protoErr != nil || proto < 0 || proto > 99999 {
		fmt.Printf("Failed to parse proto %v\n", protoStr)
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	timeNow := uint64(time.Now().UnixMilli())

	buf := make([]byte, 20)
	binary.LittleEndian.PutUint64(buf[0:], timeNow)
	binary.LittleEndian.PutUint32(buf[8:], math.Float32bits(float32(version)))
	binary.LittleEndian.PutUint32(buf[12:], uint32(platform))
	binary.LittleEndian.PutUint32(buf[16:], uint32(proto))

	encryptedData := encryptData(buf, int(version)*11+platform)
	meta := base64.StdEncoding.EncodeToString(encryptedData)

	res.Header().Set("Content-Type", "text/plain")

	serverData := "server|" + SERVER_IP + "\nport|18000\ntype2|0\n#maint|" + MAINTENANCE_MESSAGE + "\nloginurl|" + LOGIN_URL + "\nmeta|" + meta + "\nRTENDMARKERBS1001"
	res.Write([]byte(serverData))
}

func cacheHandler(res http.ResponseWriter, req *http.Request) {
	cleanPath := filepath.Clean(req.URL.Path)
	cacheRoot, _ := filepath.Abs("./static")
	filePath := filepath.Join(cacheRoot, strings.TrimPrefix(cleanPath, "/cache"))

	resolvedPath, resolvedPathErr := filepath.EvalSymlinks(filePath)
	if resolvedPathErr != nil {
		fmt.Printf("Unable to resolve %v\n", filePath)
		res.WriteHeader(http.StatusNotFound)
		return
	}

	if !strings.HasPrefix(resolvedPath, cacheRoot) {
		fmt.Printf("Blocked dangerous access %v\n", resolvedPath)
		res.WriteHeader(http.StatusNotFound)
		return
	}

	fileInfo, fileInfoErr := os.Stat(resolvedPath)
	if fileInfoErr != nil {
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	if fileInfo.IsDir() {
		res.WriteHeader(http.StatusNotFound)
		return
	}

	fileExt := strings.ToLower(filepath.Ext(resolvedPath))

	if fileExt != ".rttex" && fileExt != ".ogg" && fileExt != ".mp3" && fileExt != ".wav" {
		fmt.Printf("Trying to get not accepted file! %v\n", resolvedPath)
		res.WriteHeader(http.StatusNotFound)
		return
	}

	fmt.Printf("Serving static file %v\n", resolvedPath)
	http.ServeFile(res, req, resolvedPath)
}

func serverHandler(res http.ResponseWriter, req *http.Request) {
	if req.URL.Path == "/growtopia/server_data.php" {
		loginHandler(res, req)
		return
	}

	res.WriteHeader(http.StatusNotFound)
}

func main() {
	if SERVER_IP == "" {
		fmt.Println("SERVER_IP can not be empty!")
		return
	}

	if LOGIN_URL == "" {
		fmt.Println("WARNING: LOGIN_URL is empty, if not using newer versions it doesnt matter.")
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/", serverHandler)
	mux.HandleFunc("/cache/", cacheHandler)

	httpServer := &http.Server{
		Addr:    ":80",
		Handler: mux,
	}

	go func() {
		log.Println("HTTP server running on :80")
		log.Fatal(httpServer.ListenAndServe())
	}()

	httpsServer := &http.Server{
		Addr:    ":443",
		Handler: mux,
	}

	if fileExists("cert.pem") && fileExists("key.pem") {
		go func() {
			log.Println("HTTPS server starting on :443")
			log.Fatal(httpsServer.ListenAndServeTLS("cert.pem", "key.pem"))
		}()
	} else {
		log.Println("TLS certs not found, HTTPS not started")
	}

	select {}
}
