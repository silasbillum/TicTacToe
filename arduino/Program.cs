using arduino.Components;

var builder = WebApplication.CreateBuilder(args);

// Add services
builder.Services.AddRazorComponents()
    .AddInteractiveServerComponents();

builder.Services.AddHttpClient();
builder.Services.AddHttpClient("BlazorAPI", client =>
{
    client.BaseAddress = new Uri("https://localhost:5001/"); // Base URL for local development
});


builder.Services.AddHostedService<MqttService>();
builder.Services.AddSingleton<GameState>();
builder.Services.AddControllers();

var app = builder.Build();

// Error handling
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Error", createScopeForErrors: true);
    app.UseHsts();
}

app.UseHttpsRedirection();
app.UseStaticFiles();

app.UseRouting();

// ✅ Antiforgery must be after routing but before endpoints
app.UseAntiforgery();

app.UseEndpoints(endpoints =>
{
    endpoints.MapControllers();
});

app.MapRazorComponents<App>()
    .AddInteractiveServerRenderMode();

app.Run();
