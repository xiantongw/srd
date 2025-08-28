pipeline {
    agent {
        docker {
            image 'ubuntu:22.04'
            args '-u root:root'
        }
    }
    stages {
        stage('Install dependencies') {
            steps {
                sh '''
                    apt-get update
                    apt-get install -y curl build-essential git python3
                    curl -L https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64 -o /usr/local/bin/bazelisk
                    chmod +x /usr/local/bin/bazelisk
                '''
            }
        }
        stage('Checkout') {
            steps {
                checkout scm
            }
        }
        stage('Build') {
            steps {
                sh 'bazelisk build //...'
            }
        }
        stage('Test') {
            steps {
                sh 'bazelisk test //tests/... --test_output=all'
            }
        }
    }
    post {
        always {
            junit '**/bazel-testlogs/**/*.xml'
        }
    }
}