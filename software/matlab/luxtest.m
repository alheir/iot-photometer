

%%%                           Luxtest.m                                %%%%
%% Este Script sirve para determinar el valor de gama de un monitor
%% Hace uso de la funcion luxmeter que devuelve la luminancia recibida en lux
%% Se hace un barrido de 11 niveles de gris (de 0 a 1 en pasos de 0.1)
%% Por cada nivel se hacen Nmeas mediciones y se las promedia
%% Una vez finalizadas las mediciones se normaliza el vector de salida 
%% y se ajusta los puntos obtenidos usando CFT (curvre fitting tool)

%% Luxtest calls the following functions
%%
%% generate_gray , luxmeter
%% 



close all force
clear all

luxmed=[];                              % mediciones 
Nmeas=2;                                % Mumero de mediciones para cada valor de gris
input_level=0:0.1:1;                    % Secuencia de grises de 0 a 1 en pasos de a 0.1
k=size(input_level')                    

hwaitbar = waitbar(0,'Starting...');    % Init progress bar
movegui(hwaitbar,'northwest')
perc=0;
waitbar(perc/100,hwaitbar,sprintf('%d%% Done',perc))
perc=perc+10;

for i=1:k(1),                           %Main Loop

    lux=[];                             %Init var & window
    close 
    figure
    
    generate_gray(input_level(i))       % Open window @ given level  
     
    pause(1)                            %% Wait for window to open
    
    for n=1:Nmeas                       %% Meassure light level & average Loop
    
        lux=[lux;luxmeter]

    end
    
    
    luxmed=[luxmed;sum(lux)/Nmeas];     % average             
    
    waitbar(perc/100,hwaitbar,sprintf('%d%% Done',perc)) % Update progress
    perc=perc+10;
end

 luxmed_norm=luxmed/max(luxmed);                         % normalize 
 input_level(1)=0.00001;                                 % Fix for fitting i.e all <> 0
 [curve model] = fit(input_level',luxmed_norm,'power1')  % fit
 
 plot(curve,input_level,luxmed_norm)                     % Show ploted values
 
 gamma=curve.b                                           % Print Gamma value on plot       
 toplot=sprintf('Gamma= %.2f',gamma);
 text(.2,.6,toplot,'Color','red','FontSize',14,'FontWeight','bold');
 
 close(hwaitbar);                                        % Close progress bar
   
