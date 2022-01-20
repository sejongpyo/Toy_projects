# CNN_similar_pokemon
- 팀원: Sejong Pyo, Hanwool Choi, Soomin Kim, Woojin Lee
- 참여 기간: 2020.11.3 ~ 2020.12.18
- 목표: 닮은 동물, 연예인 등을 찾아주는 웹사이트에서 아이디어를 얻어 닮은 포켓몬을 찾아주는 웹사이트 개발
- 주요 작업 내용: 구글 이미지 크롤링, CNN 모델 구축, OpenCV를 사용한 전처리 파이프라인 구축, 웹 api

### Work Flow
1. 데이터 수집 및 전처리
- 수집 대상 선정 후 데이터 수집 진행
- 머리, 배경 등에 따른 이상치 감소를 위해 OpenCV 라이브러리를 사용해 인식 후 crop 실시
2. 모델 구축
- ImageNet pre-trained VGG-11 네트워크로 fine-tuning 실시
- fully-supervised learning으로 Top-1 80% acc 달성
3. Flask 사용한 웹 배포
- 학습 완료된 CNN 모델이 클라이언트의 요청에 따라 예측 실시 후 결과값 송출

### 결과물 예시
![image](https://user-images.githubusercontent.com/68941421/150282973-86ae966e-297e-474b-ae2e-ebdeb991a50b.png)
![image](https://user-images.githubusercontent.com/68941421/150282993-3e0e84cb-d67e-43f8-9041-ee01c70670a7.png)
