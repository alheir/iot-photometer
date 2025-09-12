function intensity = generate_gray(level)

N=600;

[X,Y]=meshgrid(1:N,1:N);

img=zeros(N,N);
for i=1:N
    for j=1:N
       
            img(i,j)=level;

    end
end

imshow(img)


end