def loadEnvironmentFromFile(path) {
	def props = readProperties file: path
	keys= props.keySet()
	for(key in keys) {
		value = props["${key}"]
		env."${key}" = "${value}"
	}
}

pipeline {
	agent any
	
	environment {
		CRASHRPT_APPID = credentials('422b61bc-371d-4cf0-b131-fffca123ddb8')
		CRASHRPT_PUBLICKEY = credentials('6e5e965d-ffd8-4cfc-9ebf-58fd1fd50930')
		SYMBOLS_OSS_FULL_ACCESS_KEY_ID = credentials('d9e2b6d7-012e-439c-8214-3bf9111bb79f')
		SYMBOLS_OSS_FULL_ACCESS_KEY_SECRET = credentials('5af89f3a-4b77-40d7-9ce9-a47970245314')
	}
	
	parameters {
		booleanParam(
			name: 'publish_package',
			defaultValue: 'false',
			description: 'Do you want to perform packaging after the build is completed?'
		)
		
		booleanParam(
			name: 'archive_symbols',
			defaultValue: 'false',
			description: 'Do you want to archive symbols after the build is completed?'
		)
	}
	
	stages {
		stage('Environment') {
			steps {
				script {
					if (env.GIT_URL.contains("Commercial")) {
						env.PUBLISH_ZIP_PASSWORD = "lovero"
					}
					loadEnvironmentFromFile("${env.JENKINS_ENVIRONMENT_FILE}")
				}

				dir('artifacts') {
					deleteDir()
				}
			}
		}
		
		stage('Prepare') {
			parallel {
				stage('Prepare: Boost') {
					steps {
						bat """
							cd 3rdparty\\boost\\
							bootstrap.bat
						"""
					}
				}
				
				stage('Prepare: Pipenv') {
					steps {
						bat """
							cd tools\\python\\
							pipenv install
						"""
					}
				}
			}
		}
		
		stage('Build') {
			steps {
				bat """
					cd tools\\python\\
					pipenv run dawn_compile.py
				"""
			}
		}
		
		stage('Finish') {
			parallel {
				stage('Finish: Archive Symbols') {
					when {
						expression {
							return params.archive_symbols
						}						
					}
					steps {
						bat """
							cd tools\\python\\
							pipenv run dawn_symstore.py
						"""
					}
				}
				
				stage('Finish: Publish Packages') {
					when {
						expression {
							return params.publish_package
						}						
					}
					steps {
						bat """
							cd tools\\python\\
							pipenv run dawn_publish.py
						"""
					}
				}
			}
		}

		stage('Archive') {
			when {
				expression {
					return params.publish_package || params.archive_symbols
				}
			}
			steps {
				archiveArtifacts artifacts: 'artifacts\\**', fingerprint: true, onlyIfSuccessful: true
			}
		}
	}

	post {
		always {
			dir('artifacts') {
				deleteDir()
			}
		}
	}
}
