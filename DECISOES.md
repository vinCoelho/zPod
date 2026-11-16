# zPod — Registro de decisões

MP3 player com "pet" virtual para uma criança de 6 anos. Projeto do zero: eletrônica, firmware e carcaça impressa em 3D.

## Premissas (definidas pelo Vinicius)

- Usuária: criança de 6 anos → simples, resistente, seguro.
- Toca MP3, navegação fácil entre músicas.
- Pet na tela que reage a: play, pausa, pular música, ligar/desligar.
- Sensor touch no topo do aparelho para acordar o pet / sair do modo de espera.
- Carcaça impressa em uma Bambu Lab A1 (volume 256 × 256 × 256 mm).
- Componentes comprados no AliExpress.

## Decisões tomadas

| # | Tema | Decisão | Motivo | Data |
|---|------|---------|--------|------|
| 1 | Saída de áudio | Alto-falante interno + fone com fio (P2). Sem Bluetooth. | Funciona sozinho, sem pareamento; fone com fio permite limitar volume. Libera o uso do ESP32-S3 (que não tem Bluetooth de áudio). | 2026-11-16 |
| 2 | Tela | IPS colorida ~2" retangular (ST7789, 240×320) | Barata, colorida, cabe pet + nome da música; animação fluida no ESP32-S3. | 2026-11-16 |
| 3 | Controles | Botões físicos grandes: play/pausa, anterior, próxima, volume +, volume − | Usável sem olhar, resistente, capas impressas coloridas. | 2026-11-16 |
| 4 | Montagem | Módulos separados soldados com fios (Vinicius solda bem) | Mais barato e flexível; carcaça otimizada. | 2026-11-16 |
| 5 | Liga/desliga | "Soft power": botão + deep sleep, sem chave física cortando a energia | O pet precisa animar ao ligar e ao desligar, o que exige o processador vivo durante a transição. (Decorrência da premissa; revisável.) | 2026-11-16 |
| 6 | Placa | ESP32-S3 DevKitC-1 N16R8 (clone) | 16 MB flash + 8 MB PSRAM, pinos de sobra, barata e fácil de repor. Carregador de bateria vai em módulo separado. | 2026-11-16 |
| 7 | Cadeia de áudio | MAX98357A (alto-falante) + PCM5102A (fone P2), no mesmo barramento I2S | Boa qualidade, barato; firmware muta o alto-falante ao detectar o fone; saída fraca do PCM5102A ajuda a limitar volume. | 2026-11-16 |
| 8 | Bateria | LiPo 603450 ~1200 mAh com proteção | Aparelho mais fino e leve; ~6–8 h de música. | 2026-11-16 |
| 9 | Entrada de músicas | microSD interno (parafusado) + envio por Wi-Fi em "modo pais" | Carcaça fechada, sem peças soltas; criança não tira o cartão. | 2026-11-16 |
| 10 | Pet | Monstrinho/bolinha original, desenhado por código (formas vetoriais, não sprites) | Animação fluida, muitas expressões com pouco trabalho de arte; a filha escolhe cor e nome. | 2026-11-16 |
| 11 | Formato | Vertical estilo iPod: tela em cima, botões em cruz embaixo, touch no topo | Cabe em mão pequena, layout interno simples. | 2026-11-16 |
| 12 | Impressão | Peças de cor única, sem AMS; contraste vem de imprimir corpo e botões em cores diferentes | Funciona em qualquer A1, sem purga. | 2026-11-16 |
| 13 | Firmware | PlatformIO + Arduino (ESP32-audioI2S para MP3, LovyanGFX para tela/pet, servidor web para upload) | Caminho mais rápido até um protótipo tocando. | 2026-11-16 |

### Decisões técnicas derivadas (recomendação do Claude — revisáveis)

| # | Tema | Decisão | Motivo |
|---|------|---------|--------|
| T1 | Carga | Módulo TP4056 USB-C com proteção; a porta USB-C dele é a única porta externa do aparelho (só carga) | Simples e seguro. A USB da DevKit fica dentro da carcaça, usada só na primeira gravação. |
| T2 | Atualização de firmware | OTA por Wi-Fi, dentro do "modo pais" | Não precisa abrir a carcaça para atualizar. |
| T3 | Regulação 3,3 V | LDO de baixa queda (RT9013 ou HT7833) da bateria direto no pino 3V3 da DevKit | O regulador da DevKit tem queda de ~1 V e não serve para LiPo. LDO bom dá ~89% de eficiência sem conversor chaveado (sem ruído no áudio). |
| T4 | Alimentação do amplificador | MAX98357A ligado direto na bateria (3,3–4,2 V) | ~1 W no alto-falante: suficiente e não alto demais para criança. |
| T5 | Touch do topo | Módulo TTP223 colado por dentro da parede superior (parede de 1,6 mm) | Saída digital simples, acorda o ESP32 do deep sleep, consome ~3 µA. Alternativa sem módulo: pino touch nativo do ESP32-S3 com fita de cobre. |
| T6 | Cartão SD | Barramento separado da tela (SD_MMC 1 bit) | Evita engasgo no áudio enquanto a tela anima. |
| T7 | Fone | Jack PJ-307 com chave de detecção + resistores de 47–100 Ω em série em cada canal | Detecta o fone para mutar o alto-falante; resistores protegem o PCM5102A e limitam o volume. |
| T8 | Limite de volume | Teto de volume em software, separado para fone e alto-falante, ajustável só no modo pais | Proteção auditiva. |
| T9 | Músicas | Cada pasta do cartão é uma "playlist"; segurar próxima/anterior troca de pasta | Navegação simples para quem ainda está aprendendo a ler. |
| T10 | Fixação | Parafusos M2,5 × 6 em insertos térmicos de latão M2,5: 4 na tampa traseira e 4 na placa dos botões | Abre e fecha muitas vezes sem gastar o plástico. |

## Riscos e pontos a validar na bancada

1. **Consumo em repouso.** A DevKit tem LED de energia e chip USB-serial sempre ligados; em deep sleep o conjunto pode gastar alguns mA e esvaziar 1200 mAh em 1–2 semanas. Medir com multímetro. Mitigações, em ordem: (a) remover o LED de energia da DevKit; (b) montar o circuito de "soft latch" (AO3401 + 2N7002), que corta a bateria de verdade ao desligar. As peças do latch já estão na lista de compras.
2. **Dimensões reais dos módulos.** Vendedores do AliExpress variam alguns milímetros. A carcaça é paramétrica (`carcaca/zpod_case.py`): medir tudo com paquímetro quando chegar e regerar os STL antes da impressão final.
3. **PCM5102A com fone.** Ele é um DAC de linha, não um amplificador de fone. Funciona bem com fones infantis, mas validar volume e distorção com o fone que ela vai usar.
4. **Bateria LiPo no AliExpress.** Envio para o Brasil às vezes é barrado ou demora; se complicar, comprar a 603450 no Mercado Livre.

## Lista de compras (AliExpress)

Preços estimados em novembro/2026, já com impostos, sem verificação anúncio a anúncio. Vários itens só vendem em kit.

| # | Item | Especificação para buscar | Qtde | Estimativa |
|---|------|---------------------------|------|-----------|
| 1 | Placa | "ESP32-S3 DevKitC-1 N16R8", **sem pinos soldados** | 1 (ideal 2) | R$ 35–50 cada |
| 2 | Tela | "2.0 inch IPS TFT ST7789V 240x320 SPI", 8 pinos | 1 | R$ 20–30 |
| 3 | Amplificador | "MAX98357A I2S amplifier module" | 1 | R$ 8–15 |
| 4 | DAC | "PCM5102A I2S DAC module" (GY-PCM5102) | 1 | R$ 15–25 |
| 5 | Alto-falante | 28 mm, 8 Ω, 2 W, até 6 mm de altura | 2 | R$ 8–15 |
| 6 | Bateria | LiPo 603450, 3,7 V, 1200 mAh, com proteção e fio | 1 | R$ 25–40 |
| 7 | Carregador | "TP4056 Type-C with protection" (2 chips) | kit 5 | R$ 8–15 |
| 8 | Regulador | "RT9013 3.3V LDO module" ou "HT7833" | 2–5 | R$ 5–12 |
| 9 | Leitor SD | "Micro SD breakout" pequeno, 3,3 V, sem regulador | 2 | R$ 5–10 |
| 10 | Cartão | microSD 16–32 GB de marca (melhor comprar no Brasil) | 1 | R$ 25–35 |
| 11 | Touch | "TTP223 touch sensor module" | kit 5–10 | R$ 5–10 |
| 12 | Botões | Chave táctil 6×6×5 mm, 4 pinos | kit 20+ | R$ 6–10 |
| 13 | Jack de fone | "PJ-307" 3,5 mm estéreo com chave | kit 10 | R$ 6–10 |
| 14 | Placa perfurada | Padrão universal 2,54 mm, 5×7 cm (cortar para os botões) | kit 5 | R$ 8–12 |
| 15 | Fio | Silicone 28–30 AWG, várias cores | kit | R$ 15–25 |
| 16 | Conectores | JST PH 2.0 de 2 vias, macho + fêmea com fio | kit 10 | R$ 6–10 |
| 17 | Fixação | Insertos térmicos M2,5 × 4 mm (Ø ext. 3,5), 8 un. + parafusos M2,5 × 6, 8 un. | kit | R$ 20–30 |
| 18 | Soft latch (opcional) | AO3401 (P-MOSFET), 2N7002, diodo 1N5819 | kits | R$ 10–15 |
| 19 | Passivos | Kit de resistores 1/4 W; capacitor eletrolítico 470 µF 10 V | kit | R$ 12–18 |

**Total estimado: R$ 250–380**, sem ferramentas (ferro de solda, multímetro, paquímetro) e sem filamento.

## Mapa de pinos (ESP32-S3 DevKitC-1 N16R8)

Evitados: 0, 3, 45, 46 (boot), 19/20 (USB), 35–37 (PSRAM), 43/44 (serial), 48 (LED RGB).

| Função | GPIO | Observação |
|--------|------|-----------|
| Tela SCK / MOSI | 12 / 11 | SPI |
| Tela CS / DC / RST | 10 / 9 / 14 | |
| Tela backlight | 47 | PWM (brilho) |
| SD CLK / CMD / D0 | 39 / 41 / 40 | SD_MMC 1 bit |
| I2S BCLK / LRCK / DOUT | 5 / 6 / 7 | Vai para os dois módulos de áudio |
| Liga/desliga do amplificador | 15 | Pino SD do MAX98357A |
| Detecção de fone | 16 | Chave do PJ-307 |
| Botão play/pausa (e ligar) | 2 | Acorda do deep sleep |
| Touch do topo (TTP223) | 4 | Acorda do deep sleep |
| Botões anterior / próxima | 17 / 18 | |
| Botões volume + / − | 8 / 21 | |
| Tensão da bateria | 1 | Divisor 100 k + 100 k |

## Pet — estados previstos (a detalhar na fase de firmware)

| Evento | Reação |
|--------|--------|
| Ligar | Acorda, espreguiça, dá oi |
| Play | Fica feliz e dança no ritmo |
| Pausa | Para, olha para ela, pisca devagar |
| Próxima / anterior | Gira ou pula para o lado correspondente |
| Volume | Olhos arregalam (sobe) / encolhe (desce) |
| Sem interação por um tempo | Boceja e dorme (modo de espera) |
| Toque no topo | Acorda com um susto fofo; se já acordado, faz cócegas |
| Bateria fraca | Fica com sono/fome |
| Desligar | Dá tchau e deita |

## Carcaça (v0.1)

Fonte paramétrica em `carcaca/zpod_case.py`; STL em `carcaca/stl/`; prévias em `carcaca/previa*.png`.

**Dimensões externas:** 66 × 114 × 23 mm, cantos com raio de 10 mm, paredes de 2 mm.

| Arquivo | Peça | Como imprimir |
|---------|------|---------------|
| `zpod_frente.stl` | Bandeja: face frontal + 4 paredes, janela da tela, furos dos botões, postes e bosses | Face frontal na mesa, sem suporte |
| `zpod_traseira.stl` | Tampa com grade e anel do alto-falante, berço do TP4056 | Face externa na mesa, sem suporte |
| `zpod_botao_*.stl` (5) | Botões com símbolo em relevo e aba de retenção | Aba na mesa, sem suporte; cor contrastante |

Sugestão: PLA ou PETG, camada 0,2 mm (0,12 mm nos botões), 3 paredes, 15% de preenchimento.

**Organização interna em camadas** (Z a partir da face frontal):

| Camada | Z (mm) | Conteúdo |
|--------|--------|----------|
| A | 2–7 | Tela (encostada na face, posicionada por 4 cantoneiras) e botões |
| B | 7–13,5 | Atrás da tela: bateria (fita dupla-face). Atrás dos botões: placa perfurada com as 5 chaves e o jack de fone, parafusada em 4 postes |
| C | 14–21 | DevKit no centro, atrás da bateria; MAX98357A, PCM5102A, leitor SD e LDO nas laterais (fita dupla-face). Na tampa: alto-falante e TP4056 |

- **Placa dos botões:** postes em grade de 2,54 mm (38,1 × 35,56 mm entre centros), para coincidir com os furos da placa perfurada (alargar 4 furos para 2,7 mm). Botões a 15,24 mm (6 furos) do play.
- **Fone:** PJ-307 soldado na mesma placa, saindo por um furo de 7 mm na parede de baixo. Encaixar inclinando a placa.
- **USB-C:** rasgo na parede de baixo; TP4056 fica entre trilhos na tampa, fixado com cola quente.
- **Touch:** rebaixo de 16 × 12 mm na parede de cima (parede fica com 1,2 mm) para colar o TTP223.
- **Tela:** soldar fios direto no módulo (sem barra de pinos) e fixar com gotas de cola quente nas cantoneiras.

**Pendências da carcaça**
- [ ] Medir tela, TP4056, jack e alto-falante reais e ajustar a seção PARÂMETROS.
- [ ] Imprimir só a frente primeiro para testar janela da tela e curso dos botões.
- [ ] Avaliar furo para cordão/alça e uma lente de proteção sobre a tela.

## Próximos passos

1. Comprar os componentes.
2. Montar tudo em protoboard e validar áudio, tela, SD e consumo em repouso.
3. Firmware: tocador básico → pet → modo pais (upload Wi-Fi + OTA).
4. Ajustar a carcaça às medidas reais e imprimir.
