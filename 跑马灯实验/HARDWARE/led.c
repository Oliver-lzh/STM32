#include "led.h"
#include "sys.h"


void Init_Led()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOE,ENABLE);//ʹ��B,Eϵ�е�ʱ��io��
	
	
	/*
		��������CRL|CRH�Ĵ�����ÿ��ϵ�ж���һ���ļĴ�����   һ����32��λ CRL����1-8  CRH����9-16
						CRLǰ�ĸ�Ϊ��PA1 ��������PA2 ��Ҫ�������������ʽ���ٶȣ�����������ٶȣ��������ٶ�ѡ��
	
	*/
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP; //������� ��Ҫ����Ϊ�������
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5;				//PB5���
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//����ٶ�
	GPIO_Init(GPIOB,&GPIO_InitStructure);					//�����úõ�ģʽ�ȴ���ú������г�ʼ�����ú����ͻ������Ӧ�ļĴ���Ϊ�������úõ�����
	GPIO_SetBits(GPIOE,GPIO_Pin_5);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP; //������� ��Ҫ����Ϊ�������
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5;				//PB5���
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//����ٶ�
	GPIO_Init(GPIOB,&GPIO_InitStructure);					//�����úõ�ģʽ�ȴ���ú������г�ʼ�����ú����ͻ������Ӧ�ļĴ���Ϊ�������úõ�����
	GPIO_SetBits(GPIOB,GPIO_Pin_5);
	/*
	IDR�Ĵ������üĴ���Ϊֻ���Ĵ���  ������ȡ��ǰ�ڵ�io״̬
	Ҫʹ�õ��ĺ�����  GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5); 
	*/
	
	/*
	ODR�Ĵ���  �üĴ���Ϊ������ݼĴ���  ������д��Ӧ�ڵĵ�ƽ״̬��ֻ�õ�16λ
	
	BSRR�Ĵ��� ��16λ�͵�16λ��������    Ҳ�����øߵ͵�ƽ

	һ�����Ƕ�io�˿ڵ�״̬���ö�������2������ GPIO_SetBits(GPIOB, GPIO_Pin_5);����Ϊ1 �ߵ�ƽ
																	GPIO_ResetBits (GPIOB, GPIO_Pin_5);����Ϊ0		 �͵�ƽ
	
	*/
	
	
	
	
	
	
}

