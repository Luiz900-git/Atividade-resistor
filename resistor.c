

 // Código modificado do repositório de Wilton Lacerda Silva - Ohmímetro utilizando o ADC da BitDogLab.
 // Neste exemplo, utilizamos o ADC do RP2040 para medir a resistência de um resistor
 // desconhecido, utilizando um divisor de tensão com dois resistores.
 // O resistor conhecido é de 10k ohm e o desconhecido é o que queremos medir.
 // Foi acrescentado uma função para exibir no display Oled a faixa de cores do resistor medido.



 // Bibliotecas base
 #include <stdio.h>
 #include <stdlib.h>
 #include "pico/stdlib.h"
 #include "hardware/adc.h"
 #include "hardware/i2c.h"
 #include "lib/ssd1306.h"
 #include "lib/font.h"
 #include <string.h>

 //Definição de gpio
 #define I2C_PORT i2c1
 #define I2C_SDA 14
 #define I2C_SCL 15
 #define endereco 0x3C
 #define ADC_PIN 28 // GPIO para o voltímetro
 #define Botao_A 5  // GPIO para botão A
 
 int R_conhecido = 10000;   // Resistor de 10k ohm
 float R_x = 0.0;           // Resistor desconhecido
 float ADC_VREF = 3.31;     // Tensão de referência do ADC
 int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)


 // NOVO: Função para obter a cor a partir do valor
const char* obter_cor(int valor) {
  switch (valor) {
      case 0: return "Preto";
      case 1: return "Marrom";
      case 2: return "Vermelho";
      case 3: return "Laranja";
      case 4: return "Amarelo";
      case 5: return "Verde";
      case 6: return "Azul";
      case 7: return "Violeta";
      case 8: return "Cinza";
      case 9: return "Branco";
      default: return "?";
  }
}

// NOVO: Função para calcular e mostrar as cores
void calcular_cores_resistor(float resistencia, char* faixa1, char* faixa2, char* faixa3) {
  int valor = (int)(resistencia + 0.5); // Arredonda

  // Trata casos muito baixos
  if (valor < 10) {
      strcpy(faixa1, obter_cor(0));
      strcpy(faixa2, obter_cor(valor));
      strcpy(faixa3, obter_cor(0));
      return;
  }

  // Reduz até obter dois dígitos significativos
  int multiplicador = 0;
  while (valor >= 100) {
      valor /= 10;
      multiplicador++;
  }

  int d1 = valor / 10;
  int d2 = valor % 10;

  strcpy(faixa1, obter_cor(d1));
  strcpy(faixa2, obter_cor(d2));
  strcpy(faixa3, obter_cor(multiplicador));
}
 
 // Trecho para modo BOOTSEL com botão B
 #include "pico/bootrom.h"
 #define botaoB 6
 void gpio_irq_handler(uint gpio, uint32_t events)
 {
   reset_usb_boot(0, 0);
 }
 
 int main()
 {
   // Para ser utilizado o modo BOOTSEL com botão B
   gpio_init(botaoB);
   gpio_set_dir(botaoB, GPIO_IN);
   gpio_pull_up(botaoB);
   gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
   // Aqui termina o trecho para modo BOOTSEL com botão B
 
   gpio_init(Botao_A);
   gpio_set_dir(Botao_A, GPIO_IN);
   gpio_pull_up(Botao_A);
 
   // I2C Initialisation. Using it at 400Khz.
   i2c_init(I2C_PORT, 400 * 1000);
 
   gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
   gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
   gpio_pull_up(I2C_SDA);                                        // Pull up the data line
   gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
   ssd1306_t ssd;                                                // Inicializa a estrutura do display
   ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
   ssd1306_config(&ssd);                                         // Configura o display
   ssd1306_send_data(&ssd);                                      // Envia os dados para o display
 
   // Limpa o display. O display inicia com todos os pixels apagados.
   ssd1306_fill(&ssd, false);
   ssd1306_send_data(&ssd);
 
   adc_init();
   adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica
 
   float tensao;
   char str_x[5]; // Buffer para armazenar a string
   char str_y[5]; // Buffer para armazenar a string
 
   bool cor = true;
   while (true)
   {
     adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica
 
     float soma = 0.0f;
     for (int i = 0; i < 500; i++)
     {
       soma += adc_read();
       sleep_ms(1);
     }
     float media = soma / 500.0f;
 
       // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
       R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
       // NOVO: Cálculo das cores
        char faixa1[10], faixa2[10], faixa3[10];
        calcular_cores_resistor(R_x, faixa1, faixa2, faixa3);
 
     sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
     sprintf(str_y, "%1.0f", R_x);   // Converte o float em string
 
     // cor = !cor;
     //  Atualiza o conteúdo do display com animações
     ssd1306_fill(&ssd, !cor);                          // Limpa o display
     ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
     ssd1306_line(&ssd, 3, 25, 123, 25, cor);           // Desenha uma linha
     ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha

     // NOVO: Exibir faixas de cor
     ssd1306_draw_string(&ssd, faixa1, 8, 6);     // Primeira faixa
     ssd1306_draw_string(&ssd, faixa2, 20, 16);    // Segunda faixa
     ssd1306_draw_string(&ssd, faixa3, 10, 28);    // Terceira faixa

     //ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
     //ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);  // Desenha uma string
     //ssd1306_draw_string(&ssd, "  Ohmimetro", 10, 28);  // Desenha uma string
     ssd1306_draw_string(&ssd, "ADC", 13, 41);          // Desenha uma string
     ssd1306_draw_string(&ssd, "Resisten.", 50, 41);    // Desenha uma string
     ssd1306_line(&ssd, 44, 37, 44, 60, cor);           // Desenha uma linha vertical
     ssd1306_draw_string(&ssd, str_x, 8, 52);           // Desenha uma string
     ssd1306_draw_string(&ssd, str_y, 59, 52);          // Desenha uma string
     
     // NOVO: Exibir faixas de cor
      //ssd1306_draw_string(&ssd, faixa1, 2, 0);     // Primeira faixa
      //ssd1306_draw_string(&ssd, faixa2, 45, 0);    // Segunda faixa
      //ssd1306_draw_string(&ssd, faixa3, 90, 0);    // Terceira faixa

     
     ssd1306_send_data(&ssd);                           // Atualiza o display
     
     sleep_ms(700);
   }
 }