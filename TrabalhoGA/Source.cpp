/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 03/10/2024
 * 
 * Readaptado por Pedro Brandelli para as atividades da disciplina
 *
 */

#include <iostream>
#include <string>
#include <assert.h>


#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <json.hpp>


using namespace std;
using json = nlohmann::json;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//STB_IMAGE
#include <stb_image.h>

// STL
#include <vector>

#include <random>
#include <algorithm>

//Classe gerenciadora de shaders
#include "Shader.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupGeometry();
int loadSimpleOBJ(string filePATH, int &nVertices);
GLuint loadTexture(string filePath, int& width, int& height);
//std::unordered_map<std::string, Material> loadMTL(const std::string& filePath);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;



// STRUCTS --------------------------------------------------------------------------

struct GeneralConfig {
	glm::vec3 cameraPos;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;
	glm::vec3 lightPos;
	glm::vec3 lightColor;
};

struct ObjectConfig {
	std::string modelPath;    // Caminho do arquivo de modelo (.obj)
	std::string texturePath;  // Caminho da textura
	std::string mtlPath;	  // Caminho do material
	glm::vec3 translation;    // Translação inicial
	glm::vec3 rotation;       // Rotação inicial (em graus)
	float scale;              // Escala inicial
	bool eMovel;			  // Para verificar se o objeto é móvel ou não
};

struct Object
{
	GLuint VAO; //Índice do buffer de geometria
	GLuint texID; //Identificador da textura carregada
	int nVertices; //nro de vértices
	glm::mat4 model; //matriz de transformações do objeto
	float ka, kd, ks; //coeficientes de iluminação - material do objeto

};

struct Curve
{
	std::vector<glm::vec3> controlPoints; // Pontos de controle da curva
	std::vector<glm::vec3> curvePoints;   // Pontos da curva
	glm::mat4 M;                          // Matriz dos coeficientes da curva
};

struct Material {
	glm::vec3 Ka;  // Coeficiente de iluminação ambiente
	glm::vec3 Kd;  // Coeficiente de iluminação difusa
	glm::vec3 Ks;  // Coeficiente de iluminação especular
	float Ns;      // Expoente especular
};



// Protótipo das funções de configuração
std::vector<ObjectConfig> loadObjectConfig(const std::string& configFile);
std::vector<GeneralConfig> loadGeneralConfig(const std::string& configFile);


// Carregando o arquivo de configuração e setando as variáveis de transformação
std::vector<GeneralConfig> Gconfigs = loadGeneralConfig("./config.json");
std::vector<ObjectConfig> configs = loadObjectConfig("./config.json");

const int NRO_OBJETOS = configs.size();

// Vetores do tamanho do número de objetos que guardam a TRANSLAÇÃO
vector<float> tx(NRO_OBJETOS, 0.0f);
vector<float> ty(NRO_OBJETOS, 0.0f);
vector<float> tz(NRO_OBJETOS, 0.0f);

// Vetor do tamanho do número de objetos que guarda a ESCALA
vector<float> fatoresEscala(NRO_OBJETOS, 1.0f);

// Vetores do tamanho do número de objetos que guardam a ROTAÇÃO
vector<float> rotateX(NRO_OBJETOS, 0.0f);
vector<float> rotateY(NRO_OBJETOS, 0.0f);
vector<float> rotateZ(NRO_OBJETOS, 0.0f);

int indice = 0;


//Funções da curva
void initializeBernsteinMatrix(glm::mat4x4& matrix);
void generateBezierCurvePoints(Curve& curve, int numPoints);
void generateGlobalBezierCurvePoints(Curve& curve, int numPoints);
void initializeCatmullRomMatrix(glm::mat4x4& matrix);
void generateCatmullRomCurvePoints(Curve& curve, int numPoints);
void displayCurve(const Curve& curve);
GLuint generateControlPointsBuffer(vector<glm::vec3> controlPoints);
std::vector<glm::vec3> generateHeartControlPoints(int numPoints);
std::vector<glm::vec3> generateInfinityControlPoints(int numPoints);



// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho GB - Pedro Brandelli", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	Shader shaderCurva = Shader("./hello-curves.vs", "./hello-curves.fs");
	Shader shader = Shader("phong.vs","phong.fs");

	Object movel;


	// Inicializando as variáveis do móvel
	int texWidth, texHeight;
	int indiceMovel = 0;
	float lastTime = 0.0;
	float FPS = 90.0;
	float angle = 0.0;
	glm::vec3 position;
	glm::vec3 dimensions;

	// Inicializando os objetos para serem renderizados
	std::vector<Object> objects(NRO_OBJETOS);
	//std::unordered_map<std::string, Material> materiais;

	for (size_t i = 0; i < NRO_OBJETOS; ++i) {
		cout << configs[i].eMovel << endl;

		if (configs[i].eMovel) {
			cout << "movel " << configs[i].modelPath << endl;
			movel.VAO = loadSimpleOBJ(configs[i].modelPath, movel.nVertices);
			movel.texID = loadTexture(configs[i].texturePath, texWidth, texHeight);
			dimensions = glm::vec3(configs[i].scale);
		}
		else {
			objects[i].VAO = loadSimpleOBJ(configs[i].modelPath, objects[i].nVertices);
			objects[i].texID = loadTexture(configs[i].texturePath, texWidth, texHeight);
			//std::unordered_map<std::string, Material> materiais = loadMTL(configs[i].mtlPath);

			tx[i] = configs[i].translation.x;
			ty[i] = configs[i].translation.y;
			tz[i] = configs[i].translation.z;
			rotateX[i] = glm::radians(configs[i].rotation.x);
			rotateY[i] = glm::radians(configs[i].rotation.y);
			rotateZ[i] = glm::radians(configs[i].rotation.z);
			fatoresEscala[i] = configs[i].scale;
			//materiais.Ka
		}
	}



	// CURVA ------------------------------

	// Estrutura para armazenar a curva de Bézier e pontos de controle
	Curve curvaBezier;
	Curve curvaCatmullRom;

	//std::vector<glm::vec3> controlPoints = generateHeartControlPoints(100);
	std::vector<glm::vec3> controlPoints = generateInfinityControlPoints(100);

	curvaBezier.controlPoints = controlPoints;

	// Para os pontos de controle da Catmull Rom precisamos duplicar o primeiro e o último
	curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[0]);
	for (int i = 0; i < curvaBezier.controlPoints.size(); i++)
	{
		curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[i]);
	}
	curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[curvaBezier.controlPoints.size() - 1]);

	// curvaBezier.controlPoints = { glm::vec3(-0.8f, -0.4f, 0.0f), glm::vec3(-0.4f, 0.4f, 0.0f),
	//                               glm::vec3(0.4f, 0.4f, 0.0f), glm::vec3(0.8f, -0.4f, 0.0f) };

	// Gerar pontos da curva de Bézier
	int numCurvePoints = 100; // Quantidade de pontos por segmento na curva
	generateGlobalBezierCurvePoints(curvaBezier, numCurvePoints);
	// generateBezierCurvePoints(curvaBezier, numCurvePoints);
	generateCatmullRomCurvePoints(curvaCatmullRom, 10);

	// Cria os buffers de geometria dos pontos da curva
	GLuint VAOControl = generateControlPointsBuffer(curvaBezier.controlPoints);
	GLuint VAOBezierCurve = generateControlPointsBuffer(curvaBezier.curvePoints);
	GLuint VAOCatmullRomCurve = generateControlPointsBuffer(curvaCatmullRom.curvePoints);

	/*cout << curvaBezier.controlPoints.size() << endl;
	cout << curvaBezier.curvePoints.size() << endl;
	cout << curvaCatmullRom.curvePoints.size() << endl;*/








	//glUseProgram(shader.ID);
	shader.Use();
	//glUseProgram(shaderCurva.ID);

	//Matriz de modelo
	glm::mat4 model = glm::mat4(1); //matriz identidade;
	GLint modelLoc = glGetUniformLocation(shader.ID, "model");
	model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	//Matriz de view
	glm::mat4 view = glm::lookAt(Gconfigs[0].cameraPos,glm::vec3(0.0f,0.0f,0.0f), Gconfigs[0].cameraUp);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	//Matriz de projeção
	//glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -1.0f, 1.0f);
	glm::mat4 projection = glm::perspective(glm::radians(39.6f),(float)WIDTH/HEIGHT,0.1f,100.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


	//Buffer de textura no shader
	glUniform1i(glGetUniformLocation(shader.ID, "texBuffer"), 0);


	glEnable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);

	//Propriedades da superfície
	shader.setFloat("ka",0.7);
	shader.setFloat("ks", 0.5);
	shader.setFloat("kd", 0.5);
	shader.setFloat("q", 10.0);

	//Propriedades da fonte de luz
	shader.setVec3("lightPos",Gconfigs[0].lightPos[0], Gconfigs[0].lightPos[1], Gconfigs[0].lightPos[2]);
	shader.setVec3("lightColor", Gconfigs[0].lightColor[0], Gconfigs[0].lightColor[1], Gconfigs[0].lightColor[2]);

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{	
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(255.0f, 255.0f, 255.0f, 1.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//shaderCurva.Use();

		glLineWidth(10);
		glPointSize(20);

		float angle = (GLfloat)glfwGetTime();


		shader.Use();

		for (size_t i = 0; i < objects.size(); i++) {

			//Propriedades da superfície
			shader.setFloat("ka", 0.7);
			shader.setFloat("ks", 0.5);
			shader.setFloat("kd", 0.5);
			shader.setFloat("q", 10.0);

			glm::mat4 model = glm::mat4(1); // Resetando a matriz para cada objeto

			//// POSIÇÃO INICIAL
			//if (i == 0) model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0f));
			//if (i == 1) model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
			//if (i == 2) model = glm::translate(model, glm::vec3(3.0f, 0.0f, 0.0f));

			// TRANSLAÇÃO
			model = glm::translate(model, glm::vec3(tx[i], ty[i], tz[i]));

			// ESCALA
			model = glm::scale(model, glm::vec3(fatoresEscala[i]));

			// ROTAÇÃO
			if (rotateX[i]) model = glm::rotate(model, rotateX[i], glm::vec3(1.0f, 0.0f, 0.0f));
			if (rotateY[i]) model = glm::rotate(model, rotateY[i], glm::vec3(0.0f, 1.0f, 0.0f));
			if (rotateZ[i]) model = glm::rotate(model, rotateZ[i], glm::vec3(0.0f, 0.0f, 1.0f));

			// Enviar matriz para o shader
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			glBindVertexArray(objects[i].VAO);
			glBindTexture(GL_TEXTURE_2D, objects[i].texID);
			glDrawArrays(GL_TRIANGLES, 0, objects[i].nVertices);

		}

		// Desenhar Móvel -----------------------------------------------------------
		position = curvaCatmullRom.curvePoints[indiceMovel];
		// Atualiza posição e rotação do objeto móvel
		float now = glfwGetTime();
		float dt = now - lastTime;

		// Atualiza o índice do ponto da curva com base no tempo e FPS
		if (dt >= 1.0f / FPS) {
			indiceMovel = (indiceMovel + 1) % curvaCatmullRom.curvePoints.size();
			lastTime = now;
		}

		// Obtém a nova posição do objeto móvel
		position = curvaCatmullRom.curvePoints[indiceMovel];

		// Calcula o próximo ponto e a direção para ajustar o ângulo
		glm::vec3 nextPos = curvaCatmullRom.curvePoints[(indiceMovel + 1) % curvaCatmullRom.curvePoints.size()];
		glm::vec3 dir = glm::normalize(nextPos - position);
		angle = atan2(dir.y, dir.x) + glm::radians(-90.0f);

		// Configura a matriz de transformações para o móvel
		movel.model = glm::mat4(1); // Matriz identidade
		movel.model = glm::translate(movel.model, position); // Translação para a posição atual
		movel.model = glm::rotate(movel.model, angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotação com base no ângulo
		movel.model = glm::scale(movel.model, dimensions); // Escala para ajustar o tamanho

		// Envia a matriz ao shader
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(movel.model));

		// Renderiza o móvel
		glBindVertexArray(movel.VAO);
		glBindTexture(GL_TEXTURE_2D, movel.texID);
		glDrawArrays(GL_TRIANGLES, 0, movel.nVertices);


		//cout << position[0] << " " << position[0] << " " << position[0];

		//Atualizar a matriz de view
		//Matriz de view
		glm::mat4 view = glm::lookAt(Gconfigs[0].cameraPos, Gconfigs[0].cameraPos + Gconfigs[0].cameraFront, Gconfigs[0].cameraUp);
		glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		//Propriedades da câmera
		shader.setVec3("cameraPos", Gconfigs[0].cameraPos.x, Gconfigs[0].cameraPos.y, Gconfigs[0].cameraPos.z);

		
		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	for (size_t i = 0; i < objects.size(); i++)
	{
		glDeleteVertexArrays(1, &objects[i].VAO);
	}
	/*glDeleteVertexArrays(1, &VAOControl);
	glDeleteVertexArrays(1, &VAOBezierCurve);
	glDeleteVertexArrays(1, &VAOCatmullRomCurve);*/

	glDeleteVertexArrays(1, &movel.VAO);

	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		indice = (indice + 1) % NRO_OBJETOS;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Rotação
	if (key == GLFW_KEY_X && action == GLFW_REPEAT)
	{
		rotateX[indice]+=0.1f;
	}

	if (key == GLFW_KEY_Y && action == GLFW_REPEAT)
	{
		rotateY[indice]+= 0.1f;
	}

	if (key == GLFW_KEY_Z && action == GLFW_REPEAT)
	{
		rotateZ[indice]+= 0.1f;
	}

	//Verifica a movimentação da câmera
	float cameraSpeed = 0.1f;

	if ((key == GLFW_KEY_UP) && action == GLFW_REPEAT)
	{
		Gconfigs[0].cameraPos += cameraSpeed * Gconfigs[0].cameraFront;
	}
	if ((key == GLFW_KEY_DOWN) && action == GLFW_REPEAT)
	{
		Gconfigs[0].cameraPos -= cameraSpeed * Gconfigs[0].cameraFront;
	}
	if ((key == GLFW_KEY_LEFT) && action == GLFW_REPEAT)
	{
		Gconfigs[0].cameraPos -= glm::normalize(glm::cross(Gconfigs[0].cameraFront, Gconfigs[0].cameraUp)) * cameraSpeed;
	}
	if ((key == GLFW_KEY_RIGHT) && action == GLFW_REPEAT)
	{
		Gconfigs[0].cameraPos += glm::normalize(glm::cross(Gconfigs[0].cameraFront, Gconfigs[0].cameraUp)) * cameraSpeed;
	}


	// Translação
	if ((key == GLFW_KEY_W) && action == GLFW_PRESS) // Cima
	{
		ty[indice]++;
	}
	if ((key == GLFW_KEY_S) && action == GLFW_PRESS) // Baixo
	{
		ty[indice]--;
	}
	if ((key == GLFW_KEY_A) && action == GLFW_PRESS) // Esquerda
	{
		tx[indice]--;
	}
	if ((key == GLFW_KEY_D) && action == GLFW_PRESS) // Direita
	{
		tx[indice]++;
	}
	if ((key == GLFW_KEY_Q) && action == GLFW_PRESS) // Frente
	{
		tz[indice]--;
	}
	if ((key == GLFW_KEY_E) && action == GLFW_PRESS) // Trás
	{
		tz[indice]++;
	}

	float passoEscala = 0.1f;
	// Escala
	if ((key == GLFW_KEY_KP_ADD) && action == GLFW_PRESS) // Frente
	{
		fatoresEscala[indice]+= passoEscala;
	}
	if ((key == GLFW_KEY_KP_SUBTRACT) && action == GLFW_PRESS) // Trás
	{
		fatoresEscala[indice]-= passoEscala;
	}



}



std::vector<GeneralConfig> loadGeneralConfig(const std::string& configFile) {
	std::vector<GeneralConfig> configs;

	std::ifstream file(configFile);
	if (!file.is_open()) {
		std::cerr << "Erro ao abrir o arquivo de configuração: " << configFile << std::endl;
		return configs;
	}

	json jsonData;
	file >> jsonData;

	//std::cout << "JSON Lido: " << jsonData.dump(4) << std::endl; // Depuração

	for (const auto& item : jsonData["camera"]) {
		GeneralConfig config;
		
		if (item.contains("cameraPos") && item["cameraPos"].is_array() && item["cameraPos"].size() == 3)
			config.cameraPos = glm::vec3(item["cameraPos"][0], item["cameraPos"][1], item["cameraPos"][2]);
		else
			config.cameraPos = glm::vec3(0.0f); // Valor padrão

		if (item.contains("cameraFront") && item["cameraFront"].is_array() && item["cameraFront"].size() == 3)
			config.cameraFront = glm::vec3(item["cameraFront"][0], item["cameraFront"][1], item["cameraFront"][2]);
		else
			config.cameraFront = glm::vec3(0.0f); // Valor padrão

		if (item.contains("cameraUp") && item["cameraUp"].is_array() && item["cameraUp"].size() == 3)
			config.cameraUp = glm::vec3(item["cameraUp"][0], item["cameraUp"][1], item["cameraUp"][2]);
		else
			config.cameraUp = glm::vec3(0.0f); // Valor padrão

		if (item.contains("lightColor") && item["lightColor"].is_array() && item["lightColor"].size() == 3)
			config.lightColor = glm::vec3(item["lightColor"][0], item["lightColor"][1], item["lightColor"][2]);
		else
			config.lightColor = glm::vec3(0.0f); // Valor padrão

		if (item.contains("lightPos") && item["lightPos"].is_array() && item["lightPos"].size() == 3)
			config.lightPos = glm::vec3(item["lightPos"][0], item["lightPos"][1], item["lightPos"][2]);
		else
			config.lightPos = glm::vec3(0.0f); // Valor padrão

		configs.push_back(config);
	}

	return configs;
}

std::vector<ObjectConfig> loadObjectConfig(const std::string& configFile) {
	std::vector<ObjectConfig> configs;

	std::ifstream file(configFile);
	if (!file.is_open()) {
		std::cerr << "Erro ao abrir o arquivo de configuração: " << configFile << std::endl;
		return configs;
	}

	json jsonData;
	file >> jsonData;

	//std::cout << "JSON Lido: " << jsonData.dump(4) << std::endl; // Depuração

	for (const auto& item : jsonData["objects"]) {
		ObjectConfig config;
		if (item.contains("modelPath"))
			config.modelPath = item["modelPath"];
		else
			config.modelPath = ""; // Valor padrão ou mensagem de erro

		if (item.contains("texturePath"))
			config.texturePath = item["texturePath"];
		else
			config.texturePath = ""; // Valor padrão ou mensagem de erro

		if (item.contains("translation") && item["translation"].is_array() && item["translation"].size() == 3)
			config.translation = glm::vec3(item["translation"][0], item["translation"][1], item["translation"][2]);
		else
			config.translation = glm::vec3(0.0f); // Valor padrão

		if (item.contains("rotation") && item["rotation"].is_array() && item["rotation"].size() == 3)
			config.rotation = glm::vec3(item["rotation"][0], item["rotation"][1], item["rotation"][2]);
		else
			config.rotation = glm::vec3(0.0f); // Valor padrão

		if (item.contains("scale"))
			config.scale = item["scale"];
		else
			config.scale = 1.0f; // Valor padrão

		if (item.contains("eMovel"))
			config.eMovel = item["eMovel"];
		else
			config.eMovel = false; // Valor padrão

		configs.push_back(config);
	}

	return configs;
}


int setupGeometry()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {

		//Base da pirâmide: 2 triângulos
		//x    y    z    r    g    b
		-0.5, -0.5, -0.5, 1.0, 1.0, 0.0,
		-0.5, -0.5,  0.5, 0.0, 1.0, 1.0,
		 0.5, -0.5, -0.5, 1.0, 0.0, 1.0,

		 -0.5, -0.5, 0.5, 1.0, 1.0, 0.0,
		  0.5, -0.5,  0.5, 0.0, 1.0, 1.0,
		  0.5, -0.5, -0.5, 1.0, 0.0, 1.0,

		 //
		 -0.5, -0.5, -0.5, 1.0, 1.0, 0.0,
		  0.0,  0.5,  0.0, 1.0, 1.0, 0.0,
		  0.5, -0.5, -0.5, 1.0, 1.0, 0.0,

		  -0.5, -0.5, -0.5, 1.0, 0.0, 1.0,
		  0.0,  0.5,  0.0, 1.0, 0.0, 1.0,
		  -0.5, -0.5, 0.5, 1.0, 0.0, 1.0,

		   -0.5, -0.5, 0.5, 1.0, 1.0, 0.0,
		  0.0,  0.5,  0.0, 1.0, 1.0, 0.0,
		  0.5, -0.5, 0.5, 1.0, 1.0, 0.0,

		   0.5, -0.5, 0.5, 0.0, 1.0, 1.0,
		  0.0,  0.5,  0.0, 0.0, 1.0, 1.0,
		  0.5, -0.5, -0.5, 0.0, 1.0, 1.0,


	};

	GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);


	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

int loadSimpleOBJ(string filePath, int& nVertices)
{
	vector <glm::vec3> vertices;
	vector <glm::vec2> texCoords;
	vector <glm::vec3> normals;
	vector <GLfloat> vBuffer;

	glm::vec3 color = glm::vec3(1.0, 0.0, 0.0);

	ifstream arqEntrada;

	arqEntrada.open(filePath.c_str());
	if (arqEntrada.is_open())
	{
		//Fazer o parsing
		string line;
		while (!arqEntrada.eof())
		{
			getline(arqEntrada, line);
			istringstream ssline(line);
			string word;
			ssline >> word;
			if (word == "v")
			{
				glm::vec3 vertice;
				ssline >> vertice.x >> vertice.y >> vertice.z;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				vertices.push_back(vertice);

			}
			if (word == "vt")
			{
				glm::vec2 vt;
				ssline >> vt.s >> vt.t;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				texCoords.push_back(vt);

			}
			if (word == "vn")
			{
				glm::vec3 normal;
				ssline >> normal.x >> normal.y >> normal.z;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				normals.push_back(normal);

			}
			else if (word == "f")
			{
				while (ssline >> word)
				{
					int vi, ti, ni;
					istringstream ss(word);
					std::string index;

					// Pega o índice do vértice
					std::getline(ss, index, '/');
					vi = std::stoi(index) - 1;  // Ajusta para índice 0

					// Pega o índice da coordenada de textura
					std::getline(ss, index, '/');
					ti = std::stoi(index) - 1;

					// Pega o índice da normal
					std::getline(ss, index);
					ni = std::stoi(index) - 1;

					//Recuperando os vértices do indice lido
					vBuffer.push_back(vertices[vi].x);
					vBuffer.push_back(vertices[vi].y);
					vBuffer.push_back(vertices[vi].z);

					//Atributo cor
					vBuffer.push_back(color.r);
					vBuffer.push_back(color.g);
					vBuffer.push_back(color.b);

					//Atributo coordenada de textura
					vBuffer.push_back(texCoords[ti].s);
					vBuffer.push_back(texCoords[ti].t);

					//Atributo vetor normal
					vBuffer.push_back(normals[ni].x);
					vBuffer.push_back(normals[ni].y);
					vBuffer.push_back(normals[ni].z);


					// Exibindo os índices para verificação
					// std::cout << "v: " << vi << ", vt: " << ti << ", vn: " << ni << std::endl;
				}

			}

		}

		arqEntrada.close();

		cout << "Gerando o buffer de geometria..." << endl;
		GLuint VBO, VAO;

		//Geração do identificador do VBO
		glGenBuffers(1, &VBO);

		//Faz a conexão (vincula) do buffer como um buffer de array
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		//Envia os dados do array de floats para o buffer da OpenGl
		glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

		//Geração do identificador do VAO (Vertex Array Object)
		glGenVertexArrays(1, &VAO);

		// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
		// e os ponteiros para os atributos 
		glBindVertexArray(VAO);

		//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
		// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
		// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
		// Tipo do dado
		// Se está normalizado (entre zero e um)
		// Tamanho em bytes 
		// Deslocamento a partir do byte zero 

		//Atributo posição (x, y, z)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		//Atributo cor (r, g, b)
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);

		//Atributo coordenada de textura - s, t
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

		//Atributo vetor normal - x, y, z
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
		glEnableVertexAttribArray(3);

		// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
		// atualmente vinculado - para que depois possamos desvincular com segurança
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
		glBindVertexArray(0);

		nVertices = vBuffer.size() / 11;
		return VAO;

	}
	else
	{
		cout << "Erro ao tentar ler o arquivo " << filePath << endl;
		return -1;
	}
}

GLuint loadTexture(string filePath, int& width, int& height)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	std::cout << width;

	return texID;
}

//std::unordered_map<std::string, Material> loadMTL(const std::string& filePath) {
//	std::unordered_map<std::string, Material> materials;
//	std::ifstream file(filePath);
//
//	if (!file.is_open()) {
//		std::cerr << "Erro ao abrir o arquivo MTL: " << filePath << std::endl;
//		return materials;
//	}
//
//	std::string line;
//	std::string currentMaterial;
//	Material material;
//
//	while (std::getline(file, line)) {
//		std::istringstream ssline(line);
//		std::string keyword;
//		ssline >> keyword;
//
//		if (keyword == "newmtl") {
//			// Salvar o material anterior, se houver
//			if (!currentMaterial.empty()) {
//				materials[currentMaterial] = material;
//			}
//			ssline >> currentMaterial; // Nome do novo material
//			material = Material();     // Reseta o material
//		}
//		else if (keyword == "Ka") {
//			// Coeficiente de iluminação ambiente
//			ssline >> material.Ka.r >> material.Ka.g >> material.Ka.b;
//		}
//		else if (keyword == "Kd") {
//			// Coeficiente de iluminação difusa
//			ssline >> material.Kd.r >> material.Kd.g >> material.Kd.b;
//		}
//		else if (keyword == "Ks") {
//			// Coeficiente de iluminação especular
//			ssline >> material.Ks.r >> material.Ks.g >> material.Ks.b;
//		}
//		else if (keyword == "Ns") {
//			// Expoente especular
//			ssline >> material.Ns;
//		}
//	}
//
//	// Adicionar o último material lido
//	if (!currentMaterial.empty()) {
//		materials[currentMaterial] = material;
//	}
//
//	file.close();
//	return materials;
//}


void initializeBernsteinMatrix(glm::mat4& matrix)
{
	// matrix[0] = glm::vec4(1.0f, -3.0f, 3.0f, -1.0f);
	// matrix[1] = glm::vec4(0.0f, 3.0f, -6.0f, 3.0f);
	// matrix[2] = glm::vec4(0.0f, 0.0f, 3.0f, -3.0f);
	// matrix[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	matrix[0] = glm::vec4(-1.0f, 3.0f, -3.0f, 1.0f); // Primeira coluna
	matrix[1] = glm::vec4(3.0f, -6.0f, 3.0f, 0.0f);  // Segunda coluna
	matrix[2] = glm::vec4(-3.0f, 3.0f, 0.0f, 0.0f);  // Terceira coluna
	matrix[3] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);   // Quarta coluna
}

void initializeCatmullRomMatrix(glm::mat4& matrix)
{
	// matrix[0] = glm::vec4(-1.0f, 3.0f, -3.0f, 1.0f);
	// matrix[1] = glm::vec4(2.0f, -5.0f, 4.0f, -1.0f);
	// matrix[2] = glm::vec4(-1.0f, 0.0f, 1.0f, 0.0f);
	// matrix[3] = glm::vec4(0.0f, 2.0f, 0.0f, 0.0f);

	matrix[0] = glm::vec4(-0.5f, 1.5f, -1.5f, 0.5f); // Primeira linha
	matrix[1] = glm::vec4(1.0f, -2.5f, 2.0f, -0.5f); // Segunda linha
	matrix[2] = glm::vec4(-0.5f, 0.0f, 0.5f, 0.0f);  // Terceira linha
	matrix[3] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);   // Quarta linha
}

void generateBezierCurvePoints(Curve& curve, int numPoints)
{
	curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

	initializeBernsteinMatrix(curve.M);
	// Calcular os pontos ao longo da curva com base em Bernstein
	// Loop sobre os pontos de controle em grupos de 4

	float piece = 1.0 / (float)numPoints;
	float t;
	for (int i = 0; i < curve.controlPoints.size() - 3; i += 3)
	{

		// Gera pontos para o segmento atual
		for (int j = 0; j < numPoints; j++)
		{
			t = j * piece;

			// Vetor t para o polinômio de Bernstein
			glm::vec4 T(t * t * t, t * t, t, 1);

			glm::vec3 P0 = curve.controlPoints[i];
			glm::vec3 P1 = curve.controlPoints[i + 1];
			glm::vec3 P2 = curve.controlPoints[i + 2];
			glm::vec3 P3 = curve.controlPoints[i + 3];

			glm::mat4x3 G(P0, P1, P2, P3);

			// Calcula o ponto da curva multiplicando tVector, a matriz de Bernstein e os pontos de controle
			glm::vec3 point = G * curve.M * T;

			curve.curvePoints.push_back(point);
		}
	}
}

void generateCatmullRomCurvePoints(Curve& curve, int numPoints)
{
	curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

	initializeCatmullRomMatrix(curve.M);
	// Calcular os pontos ao longo da curva com base em Bernstein
	// Loop sobre os pontos de controle em grupos de 4

	float piece = 1.0 / (float)numPoints;
	float t;
	for (int i = 0; i < curve.controlPoints.size() - 3; i++)
	{

		// Gera pontos para o segmento atual
		for (int j = 0; j < numPoints; j++)
		{
			t = j * piece;

			// Vetor t para o polinômio de Bernstein
			glm::vec4 T(t * t * t, t * t, t, 1);

			glm::vec3 P0 = curve.controlPoints[i];
			glm::vec3 P1 = curve.controlPoints[i + 1];
			glm::vec3 P2 = curve.controlPoints[i + 2];
			glm::vec3 P3 = curve.controlPoints[i + 3];

			glm::mat4x3 G(P0, P1, P2, P3);

			// Calcula o ponto da curva multiplicando tVector, a matriz de Bernstein e os pontos de controle
			glm::vec3 point = G * curve.M * T;
			curve.curvePoints.push_back(point);
		}
	}
}

GLuint generateControlPointsBuffer(vector<glm::vec3> controlPoints)
{
	GLuint VBO, VAO;

	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(GLfloat) * 3, controlPoints.data(), GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);

	// Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

std::vector<glm::vec3> generateHeartControlPoints(int numPoints)
{
	std::vector<glm::vec3> controlPoints;

	// Define o intervalo para t: de 0 a 2 * PI, dividido em numPoints
	float step = 2 * 3.14159 / (numPoints - 1);

	for (int i = 0; i < numPoints - 1; i++)
	{
		float t = i * step;

		// Calcula x(t) e y(t) usando as fórmulas paramétricas
		float x = 16 * pow(sin(t), 3);
		float z = 13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t);

		// Normaliza os pontos para mantê-los dentro de [-1, 1] no espaço 3D
		x /= 8.0f; // Dividir por 16 para normalizar x entre -1 e 1
		z /= 8.0f; // Dividir por 16 para normalizar y aproximadamente entre -1 e 1
		z += 0.15;
		// Adiciona o ponto ao vetor de pontos de controle
		//controlPoints.push_back(glm::vec3(x, y, 0.0f));
		controlPoints.push_back(glm::vec3(x, z, 0.0f));
	}
	controlPoints.push_back(controlPoints[0]);

	return controlPoints;
}

void generateGlobalBezierCurvePoints(Curve& curve, int numPoints)
{
	curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

	int n = curve.controlPoints.size() - 1; // Grau da curva
	float t;
	float piece = 1.0f / (float)numPoints;

	for (int j = 0; j <= numPoints; ++j)
	{
		t = j * piece;
		glm::vec3 point(0.0f); // Ponto na curva

		// Calcula o ponto da curva usando a fórmula de Bernstein
		for (int i = 0; i <= n; ++i)
		{
			// Coeficiente binomial
			float binomialCoeff = (float)(tgamma(n + 1) / (tgamma(i + 1) * tgamma(n - i + 1)));
			// Polinômio de Bernstein
			float bernsteinPoly = binomialCoeff * pow(1 - t, n - i) * pow(t, i);
			// Soma ponderada dos pontos de controle
			point += bernsteinPoly * curve.controlPoints[i];
		}

		curve.curvePoints.push_back(point);
	}
}

std::vector<glm::vec3> generateInfinityControlPoints(int numPoints) {
	std::vector<glm::vec3> controlPoints;

	// Define o intervalo para t: de 0 a 2 * PI, dividido em numPoints
	float step = 2 * 3.14159f / numPoints;

	// Constante que define o "tamanho" do símbolo
	float a = 4.0f; // Ajuste para aumentar ou diminuir o símbolo

	for (int i = 0; i < numPoints; ++i) {
		float t = i * step;

		// Fórmulas paramétricas para a lemniscata de Bernoulli
		float x = (a * sqrt(2) * cos(t)) / (sin(t) * sin(t) + 1);
		float y = (a * sqrt(2) * cos(t) * sin(t)) / (sin(t) * sin(t) + 1);

		// Adiciona o ponto no plano XY
		controlPoints.push_back(glm::vec3(x, y, 0.0f));
	}

	// Fecha o loop conectando o último ponto ao primeiro
	controlPoints.push_back(controlPoints[0]);

	return controlPoints;
}

