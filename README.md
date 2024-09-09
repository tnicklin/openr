
---

### **HTML Version**

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Open/R Containerized Setup</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
        }
        code {
            background-color: #f4f4f4;
            padding: 2px 5px;
            border-radius: 4px;
        }
        pre {
            background-color: #f4f4f4;
            padding: 10px;
            border-radius: 4px;
            overflow-x: auto;
        }
    </style>
</head>
<body>
    <h1>Open/R Containerized Setup</h1>
    <p>
        This repository contains a Dockerized version of Open/R, along with its dependencies like <code>folly</code>, 
        <code>fbthrift</code>, <code>wangle</code>, and <code>mvfst</code>. The provided Dockerfile builds all required 
        components and sets up the Open/R environment for containerized usage.
    </p>

    <h2>Prerequisites</h2>
    <p>Before starting, ensure you have the following installed on your machine:</p>
    <ul>
        <li>Docker</li>
        <li>Git</li>
    </ul>

    <h2>Clone the Repository</h2>
    <p>To get started, clone this repository and its submodules.</p>
    <pre><code>
git clone --recursive https://github.com/tnicklin/openr
cd openr
    </code></pre>
    <p>If you forgot to clone recursively, you can initialize and update the submodules with:</p>
    <pre><code>
git submodule update --init --recursive
    </code></pre>

    <h2>Build the Docker Image</h2>
    <p>To build the Docker image, use the following command:</p>
    <pre><code>
docker build -t openr-image-1 .
    </code></pre>
    <p>This command will build the Open/R image with all the necessary dependencies and configurations.</p>

    <h2>Configuration Files</h2>
    <p>
        The configuration files for each node should be placed in the <code>config</code> directory. Ensure that each 
        configuration file follows the Open/R configuration format, and the file name corresponds to the node's name, e.g., 
        <code>node_1.conf</code>.
    </p>

    <h2>Run the Container</h2>
    <p>
        Once the image is built, you can run the Open/R container. The control port is dynamically set based on the 
        provided configuration file.
    </p>
    <p>To run a node with a specific configuration file:</p>
    <pre><code>
docker run --rm --privileged --network host \
  -v /path/to/config:/config \
  openr-image-1 /scripts/run_openr.sh node_1.conf
    </code></pre>

    <h3>Running Bash in the Container</h3>
    <p>If you want to start an interactive <code>bash</code> session inside the container, use the following command:</p>
    <pre><code>
docker run -it --rm openr-image-1 bash
    </code></pre>

    <h2>Managing Ports</h2>
    <p>
        The Open/R container exposes the control port defined in the configuration file. To map the control port dynamically, 
        you can use the following command:
    </p>
    <pre><code>
docker run -e OPENR_CTRL_PORT -p $OPENR_CTRL_PORT:$OPENR_CTRL_PORT \
  -v /path/to/config:/config \
  openr-image-1 /scripts/run_openr.sh node_1.conf
    </code></pre>
    <p>Ensure the <code>OPENR_CTRL_PORT</code> is extracted from the configuration file or use a static mapping if required.</p>

    <h2>Development</h2>
    <p>
        For developers, you can modify the Open/R configuration or any of the dependencies (e.g., <code>folly</code>, 
        <code>fbthrift</code>, etc.) and rebuild the Docker image. The submodules are configured to reference specific 
        commits, but you can update these references by modifying the <code>.gitmodules</code> file or checking out 
        different versions in the <code>deps/</code> directory.
    </p>

    <h2>License</h2>
    <p>This repository and the source code it contains are licensed under the MIT License.</p>
</body>
</html>

