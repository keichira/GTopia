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

/**
* actually theres a diffirent system under it but its ok (encryption) -- it was in MathUtils proton
* and saving just random datas not actual which growtopia encrypts
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

func loginHandler(res http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodPost {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	form := req.ParseForm()
	if form != nil {
		fmt.Println("Failed to parse form")
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	if len(req.Form) == 0 {
		fmt.Println("Parsed form but its empty??")
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	versionStr := req.Form.Get("version")
	platformStr := req.Form.Get("platform")
	protoStr := req.Form.Get("protocol")

	if len(versionStr) == 0 || len(platformStr) == 0 || len(protoStr) == 0 {
		fmt.Printf("Unable to get datas version:%v platform:%v protocol:%v\n", versionStr, platformStr, protoStr)
		res.WriteHeader(http.StatusBadRequest)
	}

	version, versionErr := strconv.ParseFloat(versionStr, 32)
	if versionErr != nil || version < 0 || version > 5 {
		fmt.Printf("Failed to parse version %v\n", versionStr)
		res.WriteHeader(http.StatusBadRequest)
	}

	platform, platformErr := strconv.Atoi(platformStr)
	if platformErr != nil || platform < 0 || platform > 10 {
		fmt.Printf("Failed to parse platform %v\n", platformStr)
		res.WriteHeader(http.StatusBadRequest)
	}

	proto, protoErr := strconv.Atoi(platformStr)
	if protoErr != nil || proto < 0 || proto > 180 {
		fmt.Printf("Failed to parse platform %v\n", protoStr)
		res.WriteHeader(http.StatusBadRequest)
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

	// for now hardcoded theres only 1 login server LOL
	// also gonna update here later
	serverData := "server|192.168.1.36\nport|18000\ntype|1\n#maint|maintennace\nmeta|" + meta + "\nRTENDMARKERBS1001"
	res.Write([]byte(serverData))

	fmt.Println(serverData + "\n")
}

func cacheHandler(res http.ResponseWriter, req *http.Request) {
	cleanPath := filepath.Clean(req.URL.Path)
	cacheRoot, _ := filepath.Abs("./static")
	filePath := filepath.Join(cacheRoot, strings.TrimPrefix(cleanPath, "/cache"))

	resolvedPath, resolvedPathErr := filepath.EvalSymlinks(filePath)
	if resolvedPathErr != nil {
		fmt.Print("1")
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
	fmt.Println("Started HTTP Server")

	http.HandleFunc("/cache/", cacheHandler)
	http.HandleFunc("/", serverHandler)
	log.Fatal(http.ListenAndServe(":80", nil))
}
