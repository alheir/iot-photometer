%clear all
%close all

function lux=luxmeter()

%Serial Port Example 
instrreset                  % Ensure all ports are put in reset state (http://www.mathworks.com/help/instrument/instrreset.html)

%Create a serial object 

%ser = serial('COM1');        % Windows port style
ser = serial('/dev/ttyUSB0');   % Linux port style

% set object properties

ser.timeout=4; %% Timeout in seconds
set(ser, 'Terminator', 13); % set communication string to end on ASCII 13 % Ascii 13->CR  Ascii 10-LF (default)
set(ser, 'BaudRate', 115200);
set(ser, 'StopBits', 1);    
set(ser, 'DataBits', 8);
set(ser, 'Parity', 'none');

% Sample code for fotometer 

fopen(ser);   %% open port 

index=[];    % init variables 
answer=[];


while (isempty(index))  % Loop until selected message is received

    answer= fscanf(ser);    % wait for answer o timeout what comes first
    %%answer                  % show  response    
    index=regexp(answer, regexptranslate('wildcard','ILLUMINANCE=*')); %% if response contains ILLUMINANCE=??? (???=what ever) end loop

end


lux=str2double( answer(index+length('ILLUMINANCE')+1:end));  %% Extract Value

fclose(ser);  

end